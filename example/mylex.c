/*
 *  democli, a simple program to demonstrate how to use libocli
 *
 *  Copyright (C) 2015-2022 Digger Wu (digger.wu@linkbroad.com)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <ocli/lex.h>
#include "democli.h"

static int eth_ifnum = 0;

/*
 * ifindex is a natrual number without '0' prefix except it equals "0"
 */
int
is_ifindex(char *str)
{
	return (str && str[0] &&
		pcre_match(str, LEX_IFINDEX, "^(0|[1-9][0-9]*)$") == 1);
}

/*
 * ethernet interface name of "eth<0-x>" format
 */
int
is_eth_ifname(char *str)
{
	char	*ptr;
	int	ifindex;

	if (!str || !str[0] || eth_ifnum <= 0) return 0;
	if (strncmp(str, "eth", 3) != 0) return 0;
	if (!(*(ptr = (str + 3)))) return 0;
	if (!is_ifindex(ptr)) return 0;
	return ((ifindex = atoi(ptr)) >= 0 && ifindex < eth_ifnum);
}

static int
get_dev_ifnum(char *iftype)
{
	FILE	*fp;
	char	line[256];
	char	*tok;
	int	len, n, ifidx = -1;

	if (!iftype || !iftype[0]) return -1;
	len = strlen(iftype);

	fp = fopen("/proc/net/dev", "r");
	if (fp == NULL) {
		fprintf(stderr, "Open Linux /proc/net/dev error\n");
		return -1;
	}

	while (fgets(line, sizeof(line), fp)) {
		tok = strtok(line, " \t:");
		if (tok == NULL) continue;
		if (strncmp(tok, iftype, len) != 0) continue;

		n = atoi(tok+3);
		if (n > ifidx)
			ifidx = n;
	}
	fclose(fp);

	return ((ifidx < 0) ? 0 : (ifidx + 1));
}

int
mylex_init()
{
	char	help[80];

	eth_ifnum = get_dev_ifnum("eth");
	printf("get eth_ifnum %d\n", eth_ifnum);

	set_custom_lex_ent(LEX_IFINDEX, "IFINDEX", is_ifindex, "Interface index", NULL);

	if (eth_ifnum > 0) {
		snprintf(help, sizeof(help), "eth<0-%d>", eth_ifnum);
		set_custom_lex_ent(LEX_ETH_IFNAME, "ETH_IFNAME", is_eth_ifname, help, "eth");
	}
	return 0;
}
