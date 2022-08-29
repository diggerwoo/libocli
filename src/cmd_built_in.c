/*
 *  libocli, A general C library to provide a open-source cisco style
 *  command line interface.
 *
 *  Copyright (C) 2015 Digger Wu (digger.wu@linkbroad.com)
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

/*
 * cmd_built_in.c, provides built-in undo & manual commands
 */

#include <stdio.h>
#include "ocli.h"

static symbol_t symbols[] = {
	DEF_KEY		(UNDO_CMD,	"Undo configuration"),
	DEF_KEY		(MANUAL_CMD,	"Display manual text"),
	DEF_VAR		("COMMAND",	"Command keyword",
			 LEX_WORD,	MANUAL_ARG),
	DEF_END
};

static int cmd_manual(cmd_arg_t *cmd_arg, int do_flag);

/*
 * init undo command entry
 */
int
cmd_undo_init()
{
	struct cmd_tree *cmd_tree;

	if (prepare_symbols(&symbols[0]) < 0) {
		cleanup_symbols(&symbols[0]);
		return -1;
	}

	cmd_tree = create_cmd_tree(UNDO_CMD, &symbols[0], NULL);
	if (cmd_tree == NULL) return -1;

	add_cmd_manual(cmd_tree, UNDO_CMD" COMMAND ...", UNDO_VIEW_MASK);
	return 0;
}

/*
 * init manual command entry
 */
int
cmd_manual_init()
{
	struct cmd_tree *cmd_tree;

	if (prepare_symbols(&symbols[0]) < 0) {
		cleanup_symbols(&symbols[0]);
		return -1;
	}
	cmd_tree = create_cmd_tree(MANUAL_CMD, &symbols[0], cmd_manual);
	if (cmd_tree == NULL) return -1;

	add_cmd_manual(cmd_tree, MANUAL_CMD" COMMAND", ALL_VIEW_MASK);
	add_cmd_syntax(cmd_tree, MANUAL_CMD" COMMAND", ALL_VIEW_MASK, DO_FLAG);
	return 0;
}

#define MANUAL_BUF_SIZE	5120
/*
 * manual function entry
 */
static int
cmd_manual(cmd_arg_t *cmd_arg, int do_flag)
{
	struct cmd_tree *cmd_tree = NULL;
	int	i;
	char	*name, *value;
	int	view, res;
	char	*buf = NULL;
	char	*cmd = NULL;

	view = ocli_rl_get_view();
	
	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (strcmp(name, MANUAL_ARG) == 0)
			cmd = value;
	}

	res = get_cmd_tree(cmd, view, (DO_FLAG|UNDO_FLAG), &cmd_tree);
	if (res <= 0) {
		fprintf(stdout, "Parsing command keyword error: %s\n",
			ocli_strerror(MATCH_ERROR));
		return -1;
	} else if (res > 1) {
		fprintf(stdout, "Parsing command keyword error: %s\n",
			ocli_strerror(MATCH_AMBIGUOUS));
		return -1;
	}

	if ((buf = malloc(MANUAL_BUF_SIZE)) == NULL) {
		fprintf(stdout, "No memory for manual buf\n");
		return -1;
	}
	bzero(buf, MANUAL_BUF_SIZE);

	get_cmd_manual(cmd_tree, view, buf, MANUAL_BUF_SIZE);
	display_buf_more(buf, MANUAL_BUF_SIZE);

	free(buf);
	return 0;
}
