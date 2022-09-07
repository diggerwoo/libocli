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
#include <ocli/ocli.h>
#include "democli.h"

static char cur_ifname[32] = "";

static symbol_t syms_interface[] = {
	DEF_KEY         ("interface",	"Configure a interface"),
	DEF_VAR		("IFNAME",	"Ethernet interface name",
			 LEX_ETH_IFNAME, ARG(IFNAME))
};

static symbol_t syms_ip[] = {
	DEF_KEY         ("ip",		"IP configuration"),
	DEF_KEY         ("address",	"Set IP address"),
	DEF_VAR		("IP_ADDR",	"IP address",
			 LEX_IP_ADDR,	ARG(IP_ADDR)),
	DEF_VAR		("NET_MASK",	"Network mask",
			 LEX_IP_MASK,	ARG(NET_MASK))
};

static int cmd_interface(cmd_arg_t *cmd_arg, int do_flag);
static int cmd_ip(cmd_arg_t *cmd_arg, int do_flag);

int
cmd_interface_init()
{
	struct cmd_tree *cmd_tree;

	cmd_tree = create_cmd_tree("interface", SYM_TABLE(syms_interface), cmd_interface);
	add_cmd_easily(cmd_tree, "interface IFNAME", CONFIG_VIEW, DO_FLAG);

	cmd_tree = create_cmd_tree("ip", SYM_TABLE(syms_ip), cmd_ip);
	add_cmd_easily(cmd_tree, "ip address IP_ADDR NET_MASK", INTERFACE_VIEW, DO_FLAG);

	return 0;
}

char *
get_current_ifname()
{
	return &cur_ifname[0];
}

static int
cmd_interface(cmd_arg_t *cmd_arg, int do_flag)
{
	int	i;
	char	*name, *value;
	char	*ifname = NULL;
	int	view = ocli_rl_get_view();

	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, IFNAME))
			ifname = value;
	}

	if (ifname && ifname[0] && view == CONFIG_VIEW) {
		snprintf(cur_ifname, sizeof(cur_ifname), "%s", ifname);
		democli_set_view(INTERFACE_VIEW);
	}

	return 0;
}

static int
cmd_ip(cmd_arg_t *cmd_arg, int do_flag)
{
	int	i;
	char	*name, *value;
	char	*addr = NULL, *mask = NULL;
	char	cmd_str[128];

	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, IP_ADDR))
			addr = value;
		else if (IS_ARG(name, NET_MASK))
			mask = value;
	}

	if (cur_ifname[0] && addr && mask) {
		snprintf(cmd_str, sizeof(cmd_str),
			"ifconfig %s %s netmask %s", cur_ifname, addr, mask);
		printf("This is demo for IP address configuration.\n");
		printf("You are about to exec:\n  \"%s\"\n", cmd_str);
	}
	return 0;
}
