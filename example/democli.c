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
