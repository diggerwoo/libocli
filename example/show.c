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

static symbol_t syms_show[] = {
	DEF_KEY         ("show",	"Show utility"),
	DEF_KEY_ARG	("version",	"System version",
			 ARG(OPT_VERSION)),
	DEF_KEY_ARG	("arp",		"ARP table",
			 ARG(OPT_ARP)),
	DEF_KEY_ARG	("route",	"Route table",
			 ARG(OPT_ROUTE))
};

static int cmd_show(cmd_arg_t *cmd_arg, int do_flag);

int
cmd_show_init()
{
	struct cmd_tree *cmd_tree;

	cmd_tree = create_cmd_tree("show", SYM_TABLE(syms_show), cmd_show);
	add_cmd_easily(cmd_tree, "show version", ALL_VIEW_MASK, DO_FLAG);

	/* ARP/Route table can only be displayed above ENABLE_VIEW */
	add_cmd_easily(cmd_tree, "show { arp | route }", ENABLE_VIEW|CONFIG_VIEW, DO_FLAG);
	return 0;
}

static int
cmd_show(cmd_arg_t *cmd_arg, int do_flag)
{
	int	i;
	char	*name, *value;
	int	opt_ver = 0;
	int	opt_arp = 0, opt_route = 0;

	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, OPT_VERSION))
			opt_ver = 1;
		else if (IS_ARG(name, OPT_ARP))
			opt_arp = 1;
		else if (IS_ARG(name, OPT_ROUTE))
			opt_route = 1;
	}

	if (opt_ver)
		return(system("uname -a"));
	else if (opt_arp)
		return(system("arp -na"));
	else if (opt_route)
		return(system("route -n"));

	return 0;
}
