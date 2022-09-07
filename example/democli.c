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
#include <unistd.h>
#include <ocli/ocli.h>

int cmd_sys_init();
int cmd_enable(cmd_arg_t *cmd_arg, int do_flag);
int cmd_config(cmd_arg_t *cmd_arg, int do_flag);
int cmd_exit(cmd_arg_t *cmd_arg, int do_flag);

extern int cmd_net_utils_init();
extern int cmd_route_init();
extern int cmd_show_init();

/* A demo func to set application specific view-based prompt */
void
set_democli_prompt(int view)
{
	char	host[32];
	char	prompt[64];

	bzero(host, sizeof(host));
	gethostname(host, sizeof(host)-1);

	if (view == CONFIG_VIEW)
		snprintf(prompt, sizeof(prompt), "%s-cfg# ", host);
	else
		snprintf(prompt, sizeof(prompt),
			"%s%s ", host, (view == BASIC_VIEW) ? ">":"#");

	ocli_rl_set_prompt(prompt);
}

int
main(int argc, char **argv)
{
	/* Always init ocli_rl_init first */
	ocli_rl_init();

	/* Create libocli builtin command "man" and "no" */
	cmd_manual_init();
	cmd_undo_init();

	/* Create "enable", "configure", and "exit" commands */
	cmd_sys_init();
	/* Create "ping" and "trace-route" commands */
	cmd_net_utils_init();
	/* Create "route" command */
	cmd_route_init();
	/* Create "show" commands */
	cmd_show_init();

	/* Auto exec "exit" for EOF when CTRL-D being pressed */
	ocli_rl_set_eof_cmd("exit");

	/* For the sake of security, exit if terminal idled for 5 minutes */
	ocli_rl_set_timeout(300);

	/* Start from BASIC_VIEW */
	ocli_rl_set_view(BASIC_VIEW);
	set_democli_prompt(BASIC_VIEW);

	/* Loop to parsing and exec commands */
	ocli_rl_loop();

	/* Call ocli_rl_exit to restore original terminal attributes */
	ocli_rl_exit();
	return 0;
}

/*
 * Here we create three system commands which affect view changes:
 * "enable [password]", "configure terminal", "exit".
 */
static symbol_t syms_enable[] = {
	DEF_KEY         ("enable",	"Enabled view access"),
	DEF_KEY_ARG     ("password",	"Change password of enable view",
                         ARG(SET_PASSWD))
};

static symbol_t syms_config[] = {
	DEF_KEY         ("configure",	"Configure view access"),
	DEF_KEY		("terminal",	"Terminal mode")
};

static symbol_t syms_exit[] = {
	DEF_KEY         ("exit",	"Exit current view of democli")
};

int
cmd_sys_init()
{
	struct cmd_tree *cmd_tree;

	cmd_tree = create_cmd_tree("enable", SYM_TABLE(syms_enable), cmd_enable);

	/* BASIC_VIEW: "enable" to access ENABLE_VIEW */
	add_cmd_easily(cmd_tree, "enable", BASIC_VIEW, DO_FLAG);

	/* ENABLE_VIEW: "enable password" to update the password */
	add_cmd_easily(cmd_tree, "enable password", ENABLE_VIEW, DO_FLAG);

	cmd_tree = create_cmd_tree("configure", SYM_TABLE(syms_config), cmd_config);
	add_cmd_easily(cmd_tree, "configure terminal", ENABLE_VIEW, DO_FLAG);

	cmd_tree = create_cmd_tree("exit", SYM_TABLE(syms_exit), cmd_exit);
	add_cmd_easily(cmd_tree, "exit", ALL_VIEW_MASK, DO_FLAG);

	return 0;
}

/*
 * Callback function of "enable" command
 */
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
		printf("For demo purpose, please input \"ocli\" (without quotes) as the enable password.\n");
		passwd = read_password("Password: ");
		if (strcmp(passwd, "ocli") == 0) {
			ocli_rl_set_view(ENABLE_VIEW);
			set_democli_prompt(ENABLE_VIEW);
		} else {
			printf("Incorrect password.\n");
		}
	} else if (view == ENABLE_VIEW && set_passwd) {
		printf("This is only a demo, no password will be validated or modified.\n");
		read_password("Input old password: ");
		read_password("Input new password: ");
		read_password("Confirm new password: ");
		read_bare_line("Are you sure to modify the enable password? (Yes/No): ");
	}

	return 0;
}

/*
 * Callback function of "configure" command
 */
int
cmd_config(cmd_arg_t *cmd_arg, int do_flag)
{
	int	view = ocli_rl_get_view();

	if (view == ENABLE_VIEW) {
		ocli_rl_set_view(CONFIG_VIEW);
		set_democli_prompt(CONFIG_VIEW);
	}

	return 0;
}

/*
 * Callback function of "exit" command
 */
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
		set_democli_prompt(BASIC_VIEW);
		break;
	case CONFIG_VIEW:
		ocli_rl_set_view(ENABLE_VIEW);
		set_democli_prompt(ENABLE_VIEW);
		break;
	default:
		break;
	}
	return 0;
}
