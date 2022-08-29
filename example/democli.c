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

#include "lex.h"
#include "ocli.h"

char	cur_prompt[64] = "democli> ";

int cmd_sys_init();
int cmd_enable(cmd_arg_t *cmd_arg, int do_flag);
int cmd_exit(cmd_arg_t *cmd_arg, int do_flag);

int cmd_net_utils_init();

void
set_current_prompt(int view)
{
	if (view == BASIC_VIEW)
		snprintf(cur_prompt, sizeof(cur_prompt), "democli> ");
	else if (view == ENABLE_VIEW)
		snprintf(cur_prompt, sizeof(cur_prompt), "democli# ");
}

int
main(int argc, char **argv)
{
	char	*cmd = NULL;

	/* Always init ocli_rl_init first */
	ocli_rl_init();

	/* Init ocli readline environment */
	ocli_rl_set_auto_completion(1);

	/* for the sake of securty, exit if idled for 5 minutes */
	ocli_rl_set_timeout(300);

	/* Start with BASIC_VIEW */
	ocli_rl_set_view(BASIC_VIEW);

	/* Create libocli builtin command "man" */
	cmd_manual_init();

	/* Create "enable" and "exit" commands */
	cmd_sys_init();

	/* Create customized command "ping" and "trace-route" */
	cmd_net_utils_init();

	while (!ocli_rl_finished) {
		if ((cmd = readline(cur_prompt))) {
			if (!is_empty_line(cmd)) {
				ocli_rl_submit(cmd, ocli_rl_get_view());
				add_history(cmd);
			}
		} else {
			/* CTRL-D or idled EOF invoke "exit" command */
			rl_insert_text("exit");
			rl_redisplay();
			rl_crlf();
			ocli_rl_submit("exit", ocli_rl_get_view());
		}
	}

	/* call ocli_rl_exit before exit */
	ocli_rl_exit();
	return 0;
}

/*
 * here we create two system commands, "enable [password]" and "exit".
 */
static symbol_t symbols[] = {
	DEF_KEY         ("enable",	"Access privileged view"),
	DEF_KEY_ARG     ("password",	"Change password of privileged view",
                         ARG(SET_PASSWD)),

	DEF_KEY         ("exit",	"Exit current view of democli"),
	DEF_END
};

int
cmd_sys_init()
{
	struct cmd_tree *cmd_tree;

	if (prepare_symbols(&symbols[0]) < 0) {
		cleanup_symbols(&symbols[0]);
		return -1;
	}

	cmd_tree = create_cmd_tree("enable", &symbols[0], cmd_enable);
	add_cmd_easily(cmd_tree, "enable", BASIC_VIEW, DO_FLAG);
	add_cmd_easily(cmd_tree, "enable password", ENABLE_VIEW, DO_FLAG);

	cmd_tree = create_cmd_tree("exit", &symbols[0], cmd_exit);
	add_cmd_easily(cmd_tree, "exit", ALL_VIEW_MASK, DO_FLAG);

	return 0;
}

int
cmd_enable(cmd_arg_t *cmd_arg, int do_flag)
{
	int	i;
	char	*name, *value;
	char	*passwd = NULL;
	int	set_passwd = 0;
	int	view = ocli_rl_get_view();

	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, SET_PASSWD))
			set_passwd = 1;
	}

	if (view == BASIC_VIEW) {
		passwd = read_password("Password: ");
		if (strcmp(passwd, "ocli") == 0) {
			ocli_rl_set_view(ENABLE_VIEW);
			set_current_prompt(ENABLE_VIEW);
		} else {
			printf("For demonstration purpose, please input \"ocli\" as the enabled password\n");
		}
	} else if (view == ENABLE_VIEW && set_passwd) {
		printf("This demonstrats a command can have different syntax for different view.\n");
		printf("The password change operation should only be access in privileged view.\n");
	}

	return 0;
}

int
cmd_exit(cmd_arg_t *cmd_arg, int do_flag)
{
	int	view = ocli_rl_get_view();

	switch (view) {
	case BASIC_VIEW:
		ocli_rl_finished = 1;
		break;
	case ENABLE_VIEW:
		ocli_rl_set_view(BASIC_VIEW);
		set_current_prompt(BASIC_VIEW);
		break;
	default:
		break;
	}
	return 0;
}
