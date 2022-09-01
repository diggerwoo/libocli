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

static symbol_t syms_route[] = {
	DEF_KEY         ("route",	"Route utility"),
	DEF_VAR		("DST_NET",	"Destination network",
			 LEX_IP_ADDR,	ARG(DST_NET)),
	DEF_VAR		("DST_MASK",	"Network mask",
			 LEX_IP_MASK,	ARG(DST_MASK)),
	DEF_VAR		("GW_ADDR",	"Gateway address",
			 LEX_IP_ADDR,	ARG(GW_ADDR))
};

static int cmd_route(cmd_arg_t *cmd_arg, int do_flag);

int
cmd_route_init()
{
	struct cmd_tree *cmd_tree;

	cmd_tree = create_cmd_tree("route", SYM_TABLE(syms_route), cmd_route);

	/* Create "[no] route ... " in CONFIG_VIEW */
	add_cmd_easily(cmd_tree, "route DST_NET DST_MASK GW_ADDR", CONFIG_VIEW, DO_FLAG|UNDO_FLAG);

	return 0;
}

static int
cmd_route(cmd_arg_t *cmd_arg, int do_flag)
{
	int	i;
	char	*name, *value;
	char	net[32], mask[32], gw[32];
	char	cmd_str[128];
	

	bzero(net, sizeof(net));
	bzero(mask, sizeof(mask));
	bzero(gw, sizeof(gw));

	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, DST_NET))
			snprintf(net, sizeof(net), "%s", value);
		else if (IS_ARG(name, DST_MASK))
			snprintf(mask, sizeof(mask), "%s", value);
		else if (IS_ARG(name, GW_ADDR))
			snprintf(gw, sizeof(gw), "%s", value);
	}

	if (net[0] && mask[0] && gw[0]) {
		snprintf(cmd_str, sizeof(cmd_str),
			"route %s -net %s netmask %s gw %s",
			(do_flag == UNDO_FLAG) ? "del":"add",
			net, mask, gw);
		printf("This is demo for route command which supports \"no\" syntax.\n");
		printf("You are about to exce \"%s\".\n", cmd_str);
	}

	return 0;
}
