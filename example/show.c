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
			 ARG(OPT_ROUTE)),
	DEF_KEY_ARG	("running-config",	"Running configuration",
			 ARG(OPT_RUN_CFG)),
	DEF_KEY_ARG	("startup-config",	"Startup configuration",
			 ARG(OPT_START_CFG))
};

static int cmd_show(cmd_arg_t *cmd_arg, int do_flag);

int
cmd_show_init()
{
	struct cmd_tree *cmd_tree;

	cmd_tree = create_cmd_tree("show", SYM_TABLE(syms_show), cmd_show);
	add_cmd_easily(cmd_tree, "show version", ALL_VIEW_MASK, DO_FLAG);

	/* show syntaxes which can be only accessed above ENABLE_VIEW */
	add_cmd_easily(cmd_tree, "show { arp | route }", ENABLE_VIEW|CONFIG_VIEW, DO_FLAG);
	add_cmd_easily(cmd_tree, "show { running-config | startup-config }",
		       ENABLE_VIEW|CONFIG_VIEW, DO_FLAG);
	return 0;
}

static int
cmd_show(cmd_arg_t *cmd_arg, int do_flag)
{
	int	i;
	char	*name, *value;
	int	opt_run_cfg = 0, opt_start_cfg = 0;

	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, OPT_VERSION))
			return(system("uname -a"));
		else if (IS_ARG(name, OPT_ARP))
			return(system("arp -na"));
		else if (IS_ARG(name, OPT_ROUTE))
			return(system("route -n"));
		else if (IS_ARG(name, OPT_RUN_CFG))
			opt_run_cfg = 1;
		else if (IS_ARG(name, OPT_START_CFG))
			opt_start_cfg = 1;
	}


	if (opt_run_cfg)
		printf("!\nThis is a demo for showing running-config\n");
	else if (opt_start_cfg)
		printf("!\nThis is a demo for showing startup-config\n");

	return 0;
}
