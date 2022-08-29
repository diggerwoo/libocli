/*
 *  libocli, A general C library to provide a open-source cisco style
 *  command line interface.
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

/*
 * symbol.c, the symbol utils module of libocli
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <pcre.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ocli.h"

#define	dprintf(x, ...) \
	if ((debug_flag & x)) fprintf(stderr, __VA_ARGS__)
static int debug_flag = 0;

/*
 * reserved symbols for [ ] { | } syntax support.
 */
static symbol_t sym_builtin[] = {
	DEF_RSV	("[",	"OPT start"),
	DEF_RSV	("*",	"OPT any"),
	DEF_RSV	("]",	"OPT end"),
	DEF_RSV	("{",	"ALT start"),
	DEF_RSV	("|",	"ALT or"),
	DEF_RSV	("}",	"ALT end"),
	DEF_END
};

int sym_init_ok = 0;

/*
 * set node data for a symbol
 */
static int
set_symbol_node(symbol_t *symbol)
{
	node_t *node;
	struct lex_ent *lex;

	if (symbol->lex_type != -1 && symbol->lex_type != -2 &&
	   !IS_VALID_LEX_TYPE(symbol->lex_type)) {
		fprintf(stderr, "set_symbol_node: invalid lex type %d\n",
			symbol->lex_type);
		return -1;
	}

	if ((node = malloc(sizeof(node_t))) == NULL) {
		fprintf(stderr, "set_symbol_node: malloc failed\n");
		return -1;
	}

	bzero(node, sizeof(node_t));

	if (symbol->lex_type == -2) {
		if (strcmp(symbol->name, "[") == 0)
			node->match_type = MATCH_OPT_HEAD;
		else if (strcmp(symbol->name, "]") == 0)
			node->match_type = MATCH_OPT_END;
		else if (strcmp(symbol->name, "*") == 0)
			node->match_type = MATCH_OPT_ANY;
		else if (strcmp(symbol->name, "{") == 0)
			node->match_type = MATCH_ALT_HEAD;
		else if (strcmp(symbol->name, "}") == 0)
			node->match_type = MATCH_ALT_END;
		else if (strcmp(symbol->name, "|") == 0)
			node->match_type = MATCH_ALT_OR;
		else {
			fprintf(stderr, "set_symbol_node: bad name %s\n",
				symbol->name);
			return -1;
		}

		strncpy(node->match_ent.keyword, symbol->name, MAX_WORD_LEN-1);

	} else if (symbol->lex_type == -1) {
		node->match_type = MATCH_KEYWORD;
		strncpy(node->match_ent.keyword, symbol->name, MAX_WORD_LEN-1);

	} else {
		node->match_type = MATCH_VAR;
		node->match_ent.var.lex_type = symbol->lex_type;

		if (symbol->chk_range &&
		    IS_NUMERIC_LEX_TYPE(symbol->lex_type)) {
			node->match_ent.var.chk_range = 1;
			if (symbol->min_val <= symbol->max_val) {
				node->match_ent.var.min_val = symbol->min_val;
				node->match_ent.var.max_val = symbol->max_val;
			} else {
				node->match_ent.var.min_val = symbol->max_val;
				node->match_ent.var.max_val = symbol->min_val;
			}
		}
	}

	/* for var node, generate help if symbol->help is NULL */
	if (symbol->help && symbol->help[0]) {
		strncpy(node->help, symbol->help, MAX_TEXT_LEN-1);
	} else if (symbol->help == NULL && symbol->lex_type != -1) {
		if (symbol->chk_range) {
			if (symbol->lex_type == LEX_INT)
				snprintf(node->help, MAX_TEXT_LEN, "%d~%d",
					 (int) node->match_ent.var.min_val,
					 (int) node->match_ent.var.max_val);
			else if (symbol->lex_type == LEX_DECIMAL)
				snprintf(node->help, MAX_TEXT_LEN, "%.3f~%.3f",
					 node->match_ent.var.min_val,
					 node->match_ent.var.max_val);
		} else {
			if ((lex = get_lex_ent(symbol->lex_type)) != NULL) {
				strncpy(node->help, lex->help,
					MAX_TEXT_LEN-1);
			}
		}
	}

	/* for var node, set name as arg_name if symbol->arg_name is NULL */
	if (symbol->arg_name && symbol->arg_name[0]) {
		strncpy(node->arg_name, symbol->arg_name, MAX_WORD_LEN-1);
	} else if (symbol->arg_name == NULL && symbol->lex_type != -1) {
		strncpy(node->arg_name, symbol->name, MAX_WORD_LEN-1);
	}

	if (debug_flag) debug_node("set_symbol_node", node, 0);

	symbol->node = node;
	return 0;
}

/*
 * get symbol by name
 */
symbol_t *
get_symbol_by_name(symbol_t *symbol, char *name)
{
	while (symbol && symbol->name && symbol->name[0]) {
		if (strcmp(symbol->name, name) == 0)
			return symbol;
		symbol++;
	}
	return NULL;
}

/*
 * get symbol node pointer by name
 */
node_t *
get_node_by_name(symbol_t *symbol, char *name)
{
	symbol_t *sym;
	if (symbol && (sym = get_symbol_by_name(symbol, name)) != NULL)
		return sym->node;
	else if (!symbol && (sym = get_symbol_by_name(sym_builtin, name)) != NULL)
		return sym->node;
	else
		return NULL;
}

/*
 * scan and prepare the node_t data for each symbol
 */
int
prepare_symbols(symbol_t *symbol)
{
	while (symbol && symbol->name && symbol->name[0] &&
	       symbol->node == NULL) {
		if (set_symbol_node(symbol) < 0)
			return -1;
		symbol++;
	}

	return 0;
}

/*
 * cleanup the node_t data of symbol array
 */
int
cleanup_symbols(symbol_t *symbol)
{
	while (symbol && symbol->name) {
		if (symbol->node) {
			free(symbol->node);
			symbol->node = NULL;
		}
		symbol++;
	}

	return 0;
}

/*
 * init symbol module
 */
int
symbol_init()
{
	if (!sym_init_ok) {
		prepare_symbols(sym_builtin);
		sym_init_ok = 1;
	}
	return 1;
}

/*
 * exit symbol module
 */
void
symbol_exit()
{
	cleanup_symbols(sym_builtin);
	sym_init_ok = 0;
}
