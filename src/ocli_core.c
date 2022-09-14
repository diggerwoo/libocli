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
 * ocli_core.c, the core syntax tree module of libocli
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <pcre.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ocli.h"

#define	DBG_LIST	0x01
#define	DBG_TREE	0x02
#define	DBG_SYN		0x04

static int debug_flag = 0;
static int olic_core_init_ok = 0;
static struct list_head cmd_tree_list;

static char *err_info[] = {
	"No error",
	"No match",
	"Ambiguous match",
	"Incomplete match",
	"Too many arguments",
	"Other error",
	NULL
};

/*
 * local tree functions
 */
static void sprout_tree(node_t *tree, node_t **nodes, int num,
			int view_mask, int do_flag);
static int plant_root(node_t **root, node_t *node);
static int grow_leaf(node_t *base, int view_mask, int do_flag);
static int grow_tree(node_t *tree, node_t **nodes, int num,
			int view_mask, int do_flag);
static node_t *grow_node(node_t *base, node_t *node,
			int view_mask, int do_flag);
static int get_next_node(node_t *node, node_t **next, char *arg,
			int view, int do_flag);
static int node_has_leaf(node_t *node, int view, int do_flag);
static int node_has_only_leaf(node_t *node, int view, int do_flag);

static void debug_tree(node_t *tree, node_t **path, int len);
static void free_tree(node_t *tree);
static void free_cmd_tree(struct cmd_tree *cmd_tree);

static int set_cmd_arg(node_t *node, char *str, cmd_arg_t *cmd_arg);

/*
 * create a cmd_tree
 */
struct cmd_tree *
create_cmd_tree(char *cmd, symbol_t *sym_table, int sym_num, cmd_fun_t fun)
{
	int	res;
	node_t	*node;
	struct cmd_tree *cmd_tree, *ent;
	struct list_head *prev = NULL;

	if (!cmd || !cmd[0] || strlen(cmd) >= MAX_WORD_LEN) {
		fprintf(stderr, "create_cmd_tree: command empty or too long\n");
		return NULL;
	}

	if (!sym_table || sym_num <= 0) {
		fprintf(stderr, "create_cmd_tree: bad sym_table parm\n");
		return NULL;
	}

	if ((cmd_tree = malloc(sizeof(struct cmd_tree))) == NULL) {
		fprintf(stderr, "create_cmd_tree: no memory\n");
		return NULL;
	}

	bzero(cmd_tree, sizeof(struct cmd_tree));
	strncpy(cmd_tree->cmd, cmd, MAX_WORD_LEN-1);

	INIT_LIST_HEAD(&cmd_tree->manual_list);
	INIT_LIST_HEAD(&cmd_tree->symbol_list);

	if (prepare_symbols(&cmd_tree->symbol_list, sym_table, sym_num) < 0) {
		fprintf(stderr, "create_cmd_tree: failed to process symbols\n");
		free_cmd_tree(cmd_tree);
		return NULL;
	}

	if ((node = get_node_by_name(&cmd_tree->symbol_list, cmd)) == NULL) {
		fprintf(stderr, "create_cmd_tree: no symbol found for \'%s\'\n", cmd);
		free_cmd_tree(cmd_tree);
		return NULL;
	}

	if (plant_root(&cmd_tree->tree, node) != 0) {
		fprintf(stderr, "create_cmd_tree: set root error\n");
		free_cmd_tree(cmd_tree);
		return NULL;
	}

	cmd_tree->fun = fun;

	/* fake undo command tree */
	if (strcmp(cmd_tree->cmd, UNDO_CMD) == 0) {
		dprintf(DBG_TREE, "set flag for undo cmd_tree %s\n",
			cmd);
		cmd_tree->tree->do_view_mask = UNDO_VIEW_MASK;
		cmd_tree->tree->undo_view_mask = UNDO_VIEW_MASK;
	}

	if (list_empty(&cmd_tree_list)) {
		list_add(&cmd_tree->cmd_tree_list, &cmd_tree_list);
	} else {
		list_for_each_entry(ent, &cmd_tree_list, cmd_tree_list) {
			res = strcmp(cmd, ent->cmd);
			if (res < 0) {
				dprintf(DBG_LIST, "insert %s before %s\n",
					cmd, ent->cmd);
				prev = ent->cmd_tree_list.prev;
				list_add(&cmd_tree->cmd_tree_list, prev);
				break;
			} else if (res == 0) {
				fprintf(stderr, "create_cmd_tree: '%s' exists\n",
					cmd);
				free_cmd_tree(cmd_tree);
				return ent;
			}
		}
		if (prev == NULL) {
			dprintf(DBG_LIST, "insert %s after tail\n", cmd);
			list_add_tail(&cmd_tree->cmd_tree_list, &cmd_tree_list);
		}
	}

	return (cmd_tree);
}

/*
 * get matching command tree.
 * return number of match entries, and set the first match_tree.
 */
int
get_cmd_tree(char *cmd, int view, int do_flag, struct cmd_tree **cmd_tree)
{
	struct cmd_tree *ent, *first = NULL;
	int	n_match = 0;

	if (!cmd || !cmd[0]) return 0;

	list_for_each_entry(ent, &cmd_tree_list, cmd_tree_list) {
		/* skip UNDO_CMD if UNDO_FLAG is set */
		if (do_flag == UNDO_FLAG && strcmp(ent->cmd, UNDO_CMD) == 0)
			continue;
		if (strncmp(cmd, ent->cmd, strlen(cmd)) == 0 &&
		    ent->tree != NULL &&
		    NODE_IS_ALLOWED(ent->tree, view, do_flag)) {
			/* match exactly, quit loop */
			if (strcmp(cmd, ent->cmd) == 0) {
				first = ent;
				n_match = 1;
				break;
			}
			if (first == NULL)
				first = ent;
			n_match++;
		}
	}
	if (first != NULL) *cmd_tree = first;
	return n_match;
}

/*
 * add a manual for command tree
 */
int
add_cmd_manual(struct cmd_tree *cmd_tree, char *text, int view_mask)
{
	struct manual *manual;

	if (cmd_tree == NULL) return -1;

	if ((manual = malloc(sizeof(struct manual))) == NULL) {
		fprintf(stderr, "add_cmd_manual: no memory\n");
		return -1;
	}

	bzero(manual, sizeof(struct manual));
	strncpy(manual->text, text, MAX_MANUAL_LEN-1);
	manual->view_mask = view_mask;
	list_add_tail(&manual->manual_list, &cmd_tree->manual_list);

	return 0;
}

/*
 * get manual text for command tree
 */
int
get_cmd_manual(struct cmd_tree *cmd_tree, int view, char *buf, int limit)
{
	struct manual *man = NULL;
	char	*ptr = buf;
	int	len = 0;

	if (cmd_tree == NULL ||
	    (!(cmd_tree->tree->do_view_mask & view) &&
	     !(cmd_tree->tree->undo_view_mask & view)))
		return 0;

	if (limit > 80) {
		len = snprintf(ptr, limit, "NAME\n\t%s - %s\nSYNOPSIS\n",
			       cmd_tree->cmd,
			       cmd_tree->tree->help);
		ptr += len;
		limit -= len;
	}

	list_for_each_entry(man, &cmd_tree->manual_list, manual_list) {
		if (limit > 80 && (man->view_mask & view)) {
			len = snprintf(ptr, limit, "\t%s\n", man->text);
			ptr += len;
			limit -= len;
		}
	}
	return (ptr - buf);
}

/*
 * free all manuals
 */
void
cleanup_manuals(struct list_head *man_list)
{
	struct manual *man, *tmp;

	list_for_each_entry_safe(man, tmp, man_list, manual_list) {
		free(man);
	}
}

/*
 * add single symbol
 */
int
add_cmd_symbol(struct cmd_tree *cmd_tree, symbol_t *sym)
{
	if (!cmd_tree || !sym) return -1;

	if (get_node_by_name(NULL, sym->name) ||
	    get_node_by_name(&cmd_tree->symbol_list, sym->name))
		return -1;

	return prepare_symbols(&cmd_tree->symbol_list, sym, 1);
}

/*
 * track special syntax chars and set back is_spec flag.
 * set is_spec TRUE only if arg in "{}[*]", or is '|' inside "{...}".
 */
static void
track_syntax_char(char *arg, int *is_spec, int *in_alt)
{
	if (arg && arg[0] && !arg[1] && strchr("[*]{|}", arg[0])) {
		*is_spec = 1;
		if (arg[0] == '{')
			*in_alt = 1;
		else if (arg[0] == '}')
			*in_alt = 0;
		else if (arg[0] == '|' && *in_alt == 0)
			*is_spec = 0;
	} else
		*is_spec = 0;
}

/*
 * add a syntax into command tree
 */
int
add_cmd_syntax(struct cmd_tree *cmd_tree, char *syntax,
	       int view_mask, int do_flag)
{
	int	i, arg_num;
	char	**args = NULL;
	node_t	*nodes[MAX_ARG_NUM + 1];
	int	is_spec = 0, in_alt = 0;

	if (!cmd_tree || !syntax || !syntax[0] || !do_flag) {
		fprintf(stderr, "add_cmd_syntax: bad parm\n");
		return -1;
	}
	if ((arg_num = get_argv(syntax, &args, NULL)) <= 0) {
		fprintf(stderr, "add_cmd_syntax: zero args\n");
		return -1;
	}

	if (strcmp(cmd_tree->cmd, args[0]) != 0) {
		fprintf(stderr, "add_cmd_syntax: "
			"expect word[1] \'%s\' but get \'%s\'\n",
			cmd_tree->cmd, args[0]);
		free_argv(args);
		return -1;
	}

	bzero(&nodes[0], sizeof(nodes));

	for (i = 0; i < arg_num; i++) {
		track_syntax_char(args[i], &is_spec, &in_alt);
		nodes[i] = get_node_by_name((!is_spec) ? &cmd_tree->symbol_list : NULL, args[i]);
		if (nodes[i] == NULL) {
			fprintf(stderr, "add_cmd_syntax: "
				"bad symbol of command \'%s\', "
				"word[%d] '\%s\'\n",
				cmd_tree->cmd, i+1, args[i]);
			free_argv(args);
			return -1;
		}
	}
	free_argv(args);

	if (compare_node(cmd_tree->tree, nodes[0]) != 0) {
		fprintf(stderr, "add_cmd_syntax: weird unmatch root\n");
		return -1;
	}
	/* XXX grow from the next ! */
	return grow_tree(cmd_tree->tree, &nodes[1], arg_num-1,
			 view_mask, do_flag);
}

/*
 * one step add command syntax and manual text
 */
int
add_cmd_easily(struct cmd_tree *cmd_tree, char *syntax,
	       int view_mask, int do_flag)
{
	char	text[MAX_MANUAL_LEN];
	char	manual[MAX_MANUAL_LEN];
	char	*ptr;
	int	len = 0;
	int	zip = 0;	/* zip multi spaces as one */
	int	filter = 0;	/* filter following spaces */

	if (add_cmd_syntax(cmd_tree, syntax, view_mask, do_flag) < 0)
		return -1;

	bzero(text, sizeof(text));
	bzero(manual, sizeof(manual));

	/* zip spaces round syntax anchors */
	for (ptr = syntax; *ptr && len < MAX_MANUAL_LEN - 1; ptr++) {
		if (!isspace(*ptr)) {
			if (*ptr == '{' || *ptr == '[') {
				if (zip && !filter) text[len++] = ' ';
				filter = 1;
			} else if (*ptr == '|') {
				filter = 1;
			} else if (*ptr == '}' || *ptr == ']') {
				filter = 0;
			} else {
				if (zip && !filter) text[len++] = ' ';
				filter = 0;
			}
			text[len++] = *ptr;
			zip = 0;
		} else {
			if (!filter) zip = 1;
		}
	}

	if ((do_flag & DO_FLAG) && (do_flag & UNDO_FLAG))
		snprintf(manual, sizeof(manual), "[%s] %s", UNDO_CMD, text);
	else if ((do_flag & UNDO_FLAG))
		snprintf(manual, sizeof(manual), "%s %s", UNDO_CMD, text);
	else
		snprintf(manual, sizeof(manual), "%s", text);

	return add_cmd_manual(cmd_tree, manual, view_mask);
}

/*
 * append more for all syntaxes
 */
int
sprout_cmd_syntax(struct cmd_tree *cmd_tree, char *syntax,
		  int view_mask, int do_flag)
{
	int	i, arg_num;
	char	**args = NULL;
	node_t	*nodes[MAX_ARG_NUM + 1];
	int	is_spec = 0, in_alt = 0;

	if (!cmd_tree || !syntax || !syntax[0]) {
		fprintf(stderr, "sprout_cmd_syntax: bad parm\n");
		return -1;
	}
	if ((arg_num = get_argv(syntax, &args, NULL)) <= 0) {
		fprintf(stderr, "sprout_cmd_syntax: zero args\n");
		return -1;
	}

	bzero(&nodes[0], sizeof(nodes));

	for (i = 0; i < arg_num; i++) {
		track_syntax_char(args[i], &is_spec, &in_alt);
		nodes[i] = get_node_by_name((!is_spec) ? &cmd_tree->symbol_list : NULL, args[i]);
		if (nodes[i] == NULL) {
			fprintf(stderr, "sprout_cmd_syntax: "
				"bad symbol of command \'%s\', "
				"word[%d] '\%s\'\n",
				cmd_tree->cmd, i+1, args[i]);
			free_argv(args);
			return -1;
		}
	}
	free_argv(args);

	/* XXX sprout new nodes besides each LEAF ! */
	sprout_tree(cmd_tree->tree, &nodes[0], arg_num,
		    view_mask, do_flag);
	return 0;
}

/*
 * test if given arg matches with node.
 * return MATCH_EXACTLY (100) if key exactly match.
 */
static int
match_node(node_t *node, char *arg, int view, int do_flag)
{
	int	len;
	struct lex_ent	*lex;
	double	val;

	if (!NODE_IS_ALLOWED(node, view, do_flag))
		return 0;

	if (node->match_type == MATCH_KEYWORD) {
		len = strlen(arg);
		if (strncmp(node->match_ent.keyword, arg, len) == 0) {
			if (node->match_ent.keyword[len] == '\0')
				return MATCH_EXACTLY;
			else
				return 1;
		} else {
			return 0;
		}
	} else if (node->match_type == MATCH_VAR) {
		lex = get_lex_ent(node->match_ent.var.lex_type);
		if (lex->fun(arg) != 1) {
			return 0;
		}
		if (IS_NUMERIC_LEX_TYPE(node->match_ent.var.lex_type) &&
		    node->match_ent.var.chk_range) {
			val = atof(arg);
			if (val >= node->match_ent.var.min_val &&
			    val <= node->match_ent.var.max_val)
				return 1;
			else
				return 0;
		} else {
			return 1;
		}
	}

	return 0;
}

/*
 * recursively cleanup opt usage mark
 */
static void
cleanup_opt_mark(node_t *tree)
{
	int	i;

	if (!tree) return;
	if (tree->match_type == MATCH_OPT_HEAD)
		bzero(tree->opt_mark, sizeof(tree->opt_mark));

	for (i = 0; i < tree->branch_num; i++) {
		cleanup_opt_mark(tree->next[i]);
	}
}

/*
 * align opt mark if opt->next[k] is an ALT member
 */
static void
align_opt_mark(node_t *opt, int k)
{
	int	i;
	node_t	*node = NULL, *alt_head = NULL;

	if (!opt || opt->match_type != MATCH_OPT_HEAD ||
	    k >= opt->branch_num || !(node = opt->next[k]))
		return;

	if (node->alt_head)
		alt_head = node->alt_head;
	else if (node->alt_order == 1)
		alt_head = node;
	else
		return;

	for (i = 0; i < opt->branch_num; i++) {
		if (opt->next[i] == alt_head ||
		    opt->next[i]->alt_head == alt_head) {
			opt->opt_mark[i] = 1;
		}
	}
}

/*
 * check command syntax
 */
int
check_cmd_syntax(char *cmd, int view, cmd_stat_t *cmd_stat)
{
	int	i, arg_num, len;
	char	**args = NULL;
	int	offsets[MAX_ARG_NUM];
	int	n_match = 0;
	node_t	*node = NULL, *next = NULL;

	struct cmd_tree *cmd_tree = NULL;
	node_t	*last_node = NULL;
	int	last_argi = 0;
	int	do_flag = DO_FLAG;
	int	err_code = 0;
	int	err_argi = -1;
	int	res = 0;

	int	cmd_argi = 0;
	cmd_arg_t *cmd_arg = NULL;

	if (!cmd || !cmd[0]) {
		fprintf(stderr, "check_cmd_syntax: empty command\n");
		return -1;
	}
	if ((arg_num = get_argv(cmd, &args, &offsets[0])) <= 0) {
		fprintf(stderr, "check_cmd_syntax: zero args\n");
		return -1;
	}

	i = 0;
	len = strlen(args[0]);

	n_match = get_cmd_tree(args[i], view, do_flag, &cmd_tree);
	if (n_match == 1 && strcmp(cmd_tree->cmd, UNDO_CMD) == 0) {
		dprintf(DBG_SYN, "check undo_cmd=\'%s\'\n", cmd);
		do_flag = UNDO_FLAG; 
		if (args[1] == NULL) {
			err_code = MATCH_INCOMPLETE;
			node = cmd_tree->tree;
			last_node = node;
			last_argi = i;
			err_argi = -1;
			res = -1;
			goto check_out;
		}
		i++;
		arg_num--;
		n_match = get_cmd_tree(args[i], view, do_flag, &cmd_tree);
	}

	dprintf(DBG_SYN, "check cmd=\'%s\'\n", cmd);
	dprintf(DBG_SYN, "  try to find cmd=\'%s\' view=0x%x do=0x%x\n",
		args[i], view, do_flag);

	if (n_match > 1) {
		err_code = MATCH_AMBIGUOUS;
		err_argi = i;
		res = -1;
		goto check_out;
	} else if (n_match == 0) {
		err_code = MATCH_ERROR;
		err_argi = i;
		res = -1;
		goto check_out;
	}

	dprintf(DBG_SYN, "  n_match [%d], first \'%s\'\n",
		n_match, cmd_tree->cmd);

	if ((cmd_arg = malloc(sizeof(cmd_arg_t) * MAX_ARG_NUM)) == NULL) {
		fprintf(stderr, "check_cmd_syntax: no memory for cmd_arg\n");
		free_argv(args);
		return -1;
	}
	bzero(cmd_arg, sizeof(cmd_arg_t) * MAX_ARG_NUM);
	cmd_argi = 0;

	cleanup_opt_mark(cmd_tree->tree);

	node = cmd_tree->tree;

	/* The first command keyword can also have its cmd_arg */
	if (node->arg_name[0] && cmd_argi < MAX_ARG_NUM) {
		if (set_cmd_arg(node, args[i], &cmd_arg[cmd_argi]))
			cmd_argi++;
	}

	last_node = node;
	last_argi = i++;

	while (args[i] != NULL && node != NULL) {
		next = NULL;
		n_match = get_next_node(node, &next, args[i], view, do_flag);
		if (n_match == 1) {
			last_node = node;
			last_argi = i;

			dprintf(DBG_SYN, "  get_next_node n=%d, ", n_match);
			if ((debug_flag & DBG_SYN) && next)
				debug_node(NULL, next, 1);

			node = next;
			/* set the cmd_arg by uniq matching node */
			if (node->arg_name[0] && cmd_argi < MAX_ARG_NUM) {
				if (set_cmd_arg(node, args[i],
						&cmd_arg[cmd_argi])) {
					cmd_argi++;
				}
			}
			i++;
		} else {
			if (n_match > 1) {
				err_code = MATCH_AMBIGUOUS;
			} else if (node_has_only_leaf(node, view, do_flag)) {
				err_code = TOO_MANY_ARGS;
			} else {
				err_code = MATCH_ERROR;
			}
			free_cmd_arg(cmd_arg);
			err_argi = i;
			res = -1;
			goto check_out;
		}
	}
	if (args[i] == NULL && node != NULL) {
		n_match = node_has_leaf(node, view, do_flag);
		if (n_match == 1) {
			dprintf(DBG_SYN, "  match completely\n");
			err_code = MATCH_OK;
			cmd_stat->cmd_arg = cmd_arg;
			res = 0;
			goto check_out;
		} else {
			dprintf(DBG_SYN, "  match partially\n");
			err_code = MATCH_INCOMPLETE;
			free_cmd_arg(cmd_arg);
			err_argi = i;
			res = -1;
			goto check_out;
		}
	} else {
		err_code = MATCH_ERROR;
		free_cmd_arg(cmd_arg);
		err_argi = i;
		res = -1;
	}

check_out:
	cmd_stat->err_code = err_code;
	cmd_stat->do_flag = do_flag;
	cmd_stat->last_argi = last_argi;
	cmd_stat->last_offset = offsets[last_argi];
	if (args[last_argi])
		cmd_stat->last_arg = strdup(args[last_argi]);

	cmd_stat->err_argi = err_argi;
	if (err_argi >=0 && args[err_argi]) {
		cmd_stat->err_arg = strdup(args[err_argi]);
		cmd_stat->err_offset = offsets[err_argi];
	}

	if (cmd_tree) cmd_stat->cmd_tree = cmd_tree;
	if (last_node) cmd_stat->last_node = node;
	free_argv(args);
	return res;
}

/*
 * collect all the opt_end nodes into list
 */
static int
get_opt_end(node_t *base, node_t **node_list, int *limit, int *index)
{
	int	i;

	if (!base || *limit <= 0 || *index >= *limit) {
		fprintf(stderr, "get_opt_end: bad parm\n");
		return -1;
	}

	if (base->opt_head) {
		node_list[*index] = base;
		*limit -= 1;
		*index += 1;
		return 1;
	}
		
	for (i = 0; i < base->branch_num; i++) {
		if (get_opt_end(base->next[i], node_list, limit, index) < 0)
			return -1;
	}
	
	return 0;
}

/*
 * grow a tree with node list
 */
static int
grow_tree(node_t *tree, node_t **nodes, int num, int view_mask, int do_flag)
{
	int	i;
	node_t	*base = NULL, *ptr = NULL;

	node_t	*opt_head = NULL;	/* last opt head being parsed */
	int	opt_stat = 0;		/* '[' => 1, ']' => 2 */
	int	opt_any = 0;		/* meet '*' after '[' */

	int	alt_stat = 0;		/* '{' => 1, '}' => 2 */
	int	alt_words = 0;		/* number of word before '|' or '}' */

	int	j = 0;
	int	limit = 0, index = 0;

	node_t	*opt_base[MAX_CHOICES];	/* growth base for options */
	int	opt_num = 0;

	node_t	*alt_base[MAX_CHOICES];	/* growth base for alternatives */
	int	alt_num = 0;

	if (!tree || !nodes) {
		fprintf(stderr, "grow_tree: bad parm\n");
		return -1;
	}

	/* XXX OR the node view & do bits */
	if ((do_flag & DO_FLAG)) tree->do_view_mask |= view_mask;
	if ((do_flag & UNDO_FLAG)) tree->undo_view_mask |= view_mask;
	if ((debug_flag & DBG_TREE) && IS_ROOT(tree))
		debug_node("root .", tree, 1);

	bzero(opt_base, sizeof(opt_base));
	bzero(alt_base, sizeof(alt_base));

	base = tree;
	for (i = 0; i < num && nodes[i] != NULL; i++) {
		/*
		 * parsing for alt { | } syntax segment
		 */
		if (nodes[i]->match_type == MATCH_ALT_HEAD) {
			if (opt_stat == 2) {
				/*
				 * last OPT group ended, break for recursive growing
				 */
				break;
			}

			if (alt_stat == 1) {
				fprintf(stderr, "grow_tree: nested alt head\n");
				return -1;
			}
			alt_stat = 1;
			alt_words = 0;
			continue;

		} else if (nodes[i]->match_type == MATCH_ALT_OR) {
			if (alt_stat != 1 || alt_words != 1) {
				fprintf(stderr, "grow_tree: bad alt | position\n");
				return -1;
			}
			alt_words = 0;
			continue;

		} else if (nodes[i]->match_type == MATCH_ALT_END) {
			if (alt_stat != 1 || alt_words != 1) {
				fprintf(stderr, "grow_tree: bad alt end\n");
				return -1;
			}
			if (alt_num == 0) {
				fprintf(stderr, "grow_tree: empty alt\n");
				return -1;
			} else if (alt_num >= 2) {
				alt_base[0]->alt_order = 1;
				alt_base[0]->alt_head = NULL;
				for (j = 1; j < alt_num && alt_base[j]; j++) {
					alt_base[j]->alt_order = j + 1;
					alt_base[j]->alt_head = alt_base[0];
				}
			}

			base = alt_base[0];

			bzero(alt_base, sizeof(alt_base));
			alt_num = 0;
			alt_stat = 0;
			alt_words = 0;
			continue;

		} else if (alt_stat) {
			if (nodes[i]->match_type == MATCH_OPT_END) {
				fprintf(stderr, "grow_tree: unexpected ] in alt\n");
				return -1;
			}
			if (++alt_words != 1) {
				fprintf(stderr, "grow_tree: missing | in alt\n");
				return -1;
			}
			if ((ptr = grow_node(base, nodes[i], view_mask, do_flag)) == NULL)
				return -1;

			if (alt_num < MAX_CHOICES - 1) {
				alt_base[alt_num++] = ptr;
			} else {
				fprintf(stderr, "grow_tree: alt slot full\n");
				return -1;
			}
			continue;
		}

		/*
		 * non ALT status, parsing for opt [ ... ] and others
		 */
		if (nodes[i]->match_type == MATCH_OPT_HEAD) {
			if (opt_head) {
				/* continous [...], backtrack base to first option head */
				if (opt_stat == 2) {
					base = opt_head;
					opt_stat = 1;
					continue;
				} else if (opt_stat == 1) {
					fprintf(stderr, "grow_tree: nested opt head\n");
					return -1;
				}
			}

		} else if (nodes[i]->match_type == MATCH_OPT_ANY) {
			if (base->match_type != MATCH_OPT_HEAD) {
				fprintf(stderr, "grow_tree: bad opt * position\n");
				return -1;
			}
			opt_any = 1;
			continue;

		} else if (nodes[i]->match_type == MATCH_OPT_END) {
			if (base->match_type == MATCH_OPT_HEAD && !opt_any) {
				fprintf(stderr, "grow_tree: empty opt\n");
				return -1;
			}
			if (!opt_head || opt_stat != 1) {
				fprintf(stderr, "grow_tree: bad or nested opt end\n");
				return -1;
			}

			/* option group wildcard matched, collect all pending growing base */
			if (opt_any) {
				limit = MAX_CHOICES - opt_num,
				index = 0;
				if (get_opt_end(opt_head,
						&opt_base[opt_num],
						&limit,
						&index) < 0) {
					fprintf(stderr, "grow_tree: failed to get all opt end\n");
					return -1;
				}
				opt_stat = 2;
				opt_num += index;
				continue;
			}

			/* end of option, remember opt_head and pending growing base */
			base->opt_head = opt_head;
			opt_stat = 2;
			if (opt_num < MAX_CHOICES - 1) {
				opt_base[opt_num++] = base;
			} else {
				fprintf(stderr, "grow_tree: opt group full\n");
				return -1;
			}
			continue;

		} else {
			/* end of option group */
			if (opt_stat == 2) {
				break;
			} else if (opt_any) {
				fprintf(stderr, "grow_tree: bad opt after '*'\n");
				return -1;
			}
		}

		if ((ptr = grow_node(base, nodes[i], view_mask, do_flag)) == NULL)
			return -1;

		/* node grown as option head, mark it */
		if (ptr->match_type == MATCH_OPT_HEAD && opt_stat == 0) {
			opt_head = ptr;
			opt_stat = 1;
			/* first pending growing base with NULL option */
			opt_base[opt_num++] = base;
		}

		base = ptr;
	}

	if (opt_stat == 0 && alt_stat == 0) {
		return (grow_leaf(base, view_mask, do_flag));

	} else if (opt_stat == 2 && opt_num >= 2) {
		/* recursively grow tree on each base for remaining nodes */
		for (j = 0; j < opt_num && opt_base[j]; j++) {
			if (grow_tree(opt_base[j], &nodes[i], num - i, view_mask, do_flag) < 0) {
				return -1;
			}
		}
		return 0;

	} else if (opt_stat == 1) {
		fprintf(stderr, "grow_tree: unclosed opt clause\n");
		return -1;

	} else {
		fprintf(stderr, "grow_tree: weird, alt stat:%d num:%d, opt stat:%d num:%d\n",
			alt_stat, alt_num, opt_stat, opt_num);
		return -1;
	}
}

/*
 * plant the root node
 */
static int
plant_root(node_t **root, node_t *node)
{
	node_t	*newp;

	if (*root != NULL) {
		fprintf(stderr, "plant_root: root exists\n");
		return -1;
	}

	if (!node) {
		fprintf(stderr, "plant_root: empty node\n");
		return -1;
	}

	/* create a root node */
	if ((newp = malloc(sizeof(node_t))) == NULL) {
		fprintf(stderr, "plant_root: malloc root node error\n");
		return -1;
	}
	memcpy(newp, node, sizeof(node_t));
	newp->do_view_mask = 0;
	newp->undo_view_mask = 0;
	newp->depth = 0;
	if ((debug_flag & DBG_TREE))
		debug_node("root +", newp, 1);

	*root = newp;
	return 0;
}

/*
 * grow a leaf node
 */
static int
grow_leaf(node_t *base, int view_mask, int do_flag)
{
	int	i;
	node_t	*newp;

	if (!base) {
		fprintf(stderr, "grow_leaf: empty base or node\n");
		return -1;
	}

	/* search if given node already had a leaf */
	for (i = 0; i < base->branch_num; i++) {
		if (IS_LEAF(base->next[i])) {
			/* XXX OR the node view & do bits */
			if ((do_flag & DO_FLAG))
				base->next[i]->do_view_mask |= view_mask;
			if ((do_flag & UNDO_FLAG))
				base->next[i]->undo_view_mask |= view_mask;
			dprintf(DBG_TREE, "leaf[%d] ", i);
			if ((debug_flag & DBG_TREE))
				debug_node(".", base->next[i], 1);
			return 0;
		}
	}

	if (base->branch_num == MAX_BRANCH_NUM) {
		fprintf(stderr, "grow_leaf: no free leaf slot\n");
		return -1;
	}

	/* create a leaf node */
	if ((newp = malloc(sizeof(node_t))) == NULL) {
		fprintf(stderr, "grow_leaf: malloc root node error\n");
		return -1;
	}
	bzero(newp, sizeof(node_t));
	newp->match_type = MATCH_LEAF;
	if ((do_flag & DO_FLAG)) newp->do_view_mask = view_mask;
	if ((do_flag & UNDO_FLAG)) newp->undo_view_mask = view_mask;
	newp->depth = base->depth + 1;

	dprintf(DBG_TREE, "leaf[%d] ", base->branch_num);
	if ((debug_flag & DBG_TREE))
		debug_node("+", newp, 1);

	base->next[base->branch_num++] = newp;
	return 0;
}

/*
 * grow one new node from base
 */
static node_t *
grow_node(node_t *base, node_t *node, int view_mask, int do_flag)
{
	node_t	*newp;
	int	i;

	if (!base || !node) {
		fprintf(stderr, "grow_node: empty base or node\n");
		return NULL;
	}

	/* search if given node match with a branch */
	for (i = 0; i < base->branch_num; i++) {
		if (compare_node(node, base->next[i]) == 0) {
			/* XXX OR the node view & do bits */
			if ((do_flag & DO_FLAG))
				base->next[i]->do_view_mask |= view_mask;
			if ((do_flag & UNDO_FLAG))
				base->next[i]->undo_view_mask |= view_mask;
			dprintf(DBG_TREE, "next[%d] ", i);
			if ((debug_flag & DBG_TREE))
				debug_node(".", base->next[i], 1);

			return (base->next[i]);
		}
	}

	if (base->branch_num == MAX_BRANCH_NUM) {
		fprintf(stderr, "grow_node: no free branch slot\n");
		return NULL;
	}

	/* create a branch node */
	if ((newp = malloc(sizeof(node_t))) == NULL) {
		fprintf(stderr, "grow_node: malloc new node error\n");
		return NULL;
	}
	memcpy(newp, node, sizeof(node_t));
	if ((do_flag & DO_FLAG)) newp->do_view_mask = view_mask;
	if ((do_flag & UNDO_FLAG)) newp->undo_view_mask = view_mask;
	newp->depth = base->depth + 1;

	dprintf(DBG_TREE, "next[%d] ", base->branch_num);
	if ((debug_flag & DBG_TREE))
		debug_node("+", newp, 1);

	base->next[base->branch_num++] = newp;
	return newp;
}

/*
 * get strings from node partialy matches with cmd
 */
int
get_node_matches(node_t *node, char *cmd, char **matches, int limit,
		 int view, int do_flag)
{
	int	n_match = 0;
	char	pfx[MAX_WORD_LEN];
	struct cmd_tree *ent = NULL;
	struct lex_ent *lex = NULL;

	/* node NULL, or manual arg var, list all matching commands */
	if (node == NULL ||
	    (node->match_type == MATCH_VAR &&
	     NODE_IS_ALLOWED(node, view, do_flag) &&
	     node->match_ent.var.lex_type == LEX_WORD &&
	     strcmp(node->arg_name, MANUAL_ARG) == 0)) {
		list_for_each_entry(ent, &cmd_tree_list, cmd_tree_list) {
			if (ent->tree != NULL &&
			    NODE_IS_ALLOWED(ent->tree, view, do_flag) &&
			    (!cmd || !cmd[0] ||
			     strncmp(cmd, ent->cmd, strlen(cmd)) == 0)) {
				if (n_match < limit)
					matches[n_match++] = strdup(ent->cmd);
				else
					break;
			}
		}
		return n_match;
	} 

	/* keyword node */
	if (node->match_type == MATCH_KEYWORD &&
	    NODE_IS_ALLOWED(node, view, do_flag) &&
	    (!cmd || !cmd[0] ||
	     strncmp(node->match_ent.keyword, cmd, strlen(cmd)) == 0)) {
		if (limit >= 1) {
			matches[0] = strdup(node->match_ent.keyword);
			return 1;
		} else
			return 0;
	}

	/* var node full match, or partially match with prefix */
	if (node->match_type == MATCH_VAR &&
	    NODE_IS_ALLOWED(node, view, do_flag) &&
	    (lex = get_lex_ent(node->match_ent.var.lex_type))) {
		if (cmd && cmd[0] && 
		    !node->arg_helper && limit >= 1 &&
		    lex->fun(cmd) == 1) {
			matches[0] = strdup(cmd);
			return 1;
		} else if (node->arg_helper && limit >= 1) {
			return node->arg_helper(cmd, matches, limit);
		} else if (lex->prefix[0] &&
		           (!cmd || !cmd[0] ||
		           strncmp(lex->prefix, cmd, strlen(cmd)) == 0)) {
			/* precedent ^ to mark prefix match */
			snprintf(pfx, sizeof(pfx), "^%s", lex->prefix);
			matches[n_match++] = strdup(pfx);
			return n_match;
		}
	}
	    
	return 0;
}

/*
 * get partially matched strings from all branches of node
 */
int
get_node_next_matches(node_t *node, char *cmd, char **matches, int limit,
		      int view, int do_flag)
{
	int	i, j, n_match = 0;
	node_t	*opt = NULL;
	struct cmd_tree *ent = NULL;

	if (!node) return 0;

	/* if node is UNDO node, list matching commands support undo */
	if (node->match_type == MATCH_KEYWORD &&
	    NODE_IS_ALLOWED(node, view, do_flag) &&
	    strcmp(node->match_ent.keyword, UNDO_CMD) == 0 &&
	    (!cmd || !cmd[0])) {
		list_for_each_entry(ent, &cmd_tree_list, cmd_tree_list) {
			if (ent->tree != NULL &&
			    NODE_IS_ALLOWED(ent->tree, view, do_flag) &&
			    strcmp(ent->cmd, UNDO_CMD) != 0) {
				if (n_match < limit)
					matches[n_match++] = strdup(ent->cmd);
				else
					break;
			}
		}
		return n_match;
	}

	/* if node is MANUAL node, list all commands */
	if (node->match_type == MATCH_KEYWORD &&
	    NODE_IS_ALLOWED(node, view, do_flag) &&
	    strcmp(node->match_ent.keyword, MANUAL_CMD) == 0 &&
	    (!cmd || !cmd[0])) {
		list_for_each_entry(ent, &cmd_tree_list, cmd_tree_list) {
			if (ent->tree != NULL &&
			    NODE_IS_ALLOWED(ent->tree, view, do_flag)) {
				if (n_match < limit)
					matches[n_match++] = strdup(ent->cmd);
				else
					break;
			}
		}
		return n_match;
	}

	if (node->alt_head)
		node = node->alt_head;

	for (i = 0; i < node->branch_num && n_match < limit; i++) {
		if (node->next[i]->match_type == MATCH_OPT_HEAD)
			opt = node->next[i];
		else if (node->opt_head)
			opt = node->opt_head;
		else
			opt = NULL;

		if (opt) {
			for (j = 0; j < opt->branch_num; j++) {
				if (opt->opt_mark[j])
					continue;

				n_match += get_node_matches(opt->next[j], cmd,
							    &matches[n_match], limit - n_match,
							    view, do_flag);
			}
		}

		n_match += get_node_matches(node->next[i], cmd,
					    &matches[n_match], limit - n_match,
					    view, do_flag);
	}
	return n_match;
}

/*
 * get help info from node partialy matches with cmd
 */
int
get_node_help(node_t *node, char *cmd, char *buf, int limit,
	      int view, int do_flag)
{
	struct cmd_tree *ent = NULL;
	struct lex_ent *lex = NULL;
	char	*ptr = buf;
	int	len = 0;

	/* node NULL, list all matching commands */
	if (node == NULL) {
		list_for_each_entry(ent, &cmd_tree_list, cmd_tree_list) {
			if (ent->tree != NULL &&
			    NODE_IS_ALLOWED(ent->tree, view, do_flag) &&
			    (!cmd || !cmd[0] ||
			     strncmp(cmd, ent->cmd, strlen(cmd)) == 0)) {
				len = snprintf(ptr, limit, "  %-22s - %s\n",
					       ent->cmd, ent->tree->help);
				ptr += len;
				limit -= len;
				if (limit < 50) break;
			}
		}
		return (ptr - buf);
	} 

	/* keyword node */
	if (node->match_type == MATCH_KEYWORD &&
	    NODE_IS_ALLOWED(node, view, do_flag) &&
	    (!cmd || !cmd[0] ||
	     strncmp(node->match_ent.keyword, cmd, strlen(cmd)) == 0)) {
		len = snprintf(ptr, limit, "  %-22s - %s\n",
			       node->match_ent.keyword, node->help);
		ptr += len;
		limit -= len;
		return (ptr - buf);
	}

	/* var node */
	if (node->match_type == MATCH_VAR &&
	    NODE_IS_ALLOWED(node, view, do_flag) &&
	    (lex = get_lex_ent(node->match_ent.var.lex_type))) {
		if (!cmd || !cmd[0] ||
		    lex->fun(cmd) == 1 ||
		    (lex->prefix[0] && 
		     strncmp(cmd, lex->prefix, strlen(cmd)) == 0)) {
			len = snprintf(ptr, limit, "  %-22s - %s\n",
				       lex->help, node->help);
			ptr += len;
			limit -= len;
			return (ptr - buf);
		}
	}
	    
	/* leaf node */
	if (IS_LEAF(node) &&
	    NODE_IS_ALLOWED(node, view, do_flag)) {
		len = snprintf(ptr, limit, "  %-22s - %s\n",
			       "<Enter>", "End of command");
		ptr += len;
		limit -= len;
		return (ptr - buf);
	}

	return 0;
}

/*
 * get help strings from all branches of node
 */
int
get_node_next_help(node_t *node, char *cmd, char *buf, int limit,
		   int view, int do_flag)
{
	int	i, j;
	char	*ptr = buf;
	int	len = 0;
	node_t	*opt = NULL;
	struct cmd_tree *ent = NULL;

	if (!node) return 0;

	/* if node is UNDO node, list matching commands support undo */
	if (node->match_type == MATCH_KEYWORD &&
	    NODE_IS_ALLOWED(node, view, do_flag) &&
	    strcmp(node->match_ent.keyword, UNDO_CMD) == 0) {
		list_for_each_entry(ent, &cmd_tree_list, cmd_tree_list) {
			if (ent->tree != NULL &&
			    NODE_IS_ALLOWED(ent->tree, view, do_flag) &&
			    (!cmd || !cmd[0] ||
			     strncmp(cmd, ent->cmd, strlen(cmd)) == 0) &&
			    strcmp(ent->cmd, UNDO_CMD) != 0) {
				len = snprintf(ptr, limit, "  %-22s - %s\n",
					       ent->cmd, ent->tree->help);
				ptr += len;
				limit -= len;
				if (limit < 50) break;
			}
		}
		return (ptr - buf);
	}

	if (node->alt_head)
		node = node->alt_head;

	for (i = 0; i < node->branch_num; i++) {
		if (node->next[i]->match_type == MATCH_OPT_HEAD)
			opt = node->next[i];
		else if (node->opt_head)
			opt = node->opt_head;
		else
			opt = NULL;

		if (opt) {
			for (j = 0; j < opt->branch_num; j++) {
				if (opt->opt_mark[j])
					continue;

				len = get_node_help(opt->next[j], cmd,
						    ptr, limit,
						    view, do_flag);
				opt->opt_mark[j] = 1;
				ptr += len;
				limit -= len;
				if (limit < 50) goto out;
			}
		}

		len = get_node_help(node->next[i], cmd,
				    ptr, limit,
				    view, do_flag);
		ptr += len;
		limit -= len;
		if (limit < 50) break;
	}
out:
	return (ptr - buf);
}

/*
 * compare two nodes, return 0 if equal
 */
int compare_node(node_t *node1, node_t *node2)
{
	if (!node1 || !node2) return -1;

	if (node1->match_type != node2->match_type)
		return -1;

	if (node1->match_type == MATCH_KEYWORD)
		return strcmp(node1->match_ent.keyword,
			      node2->match_ent.keyword);
	else if (node1->match_type == MATCH_VAR)
		return (node1->match_ent.var.lex_type -
			node2->match_ent.var.lex_type);
	else 
		return 0;
}

/*
 * check if node has a matching leaf
 */
static int
node_has_leaf(node_t *node, int view, int do_flag)
{
	int	i;
	int	max_tries = 2;

	if (!node) {
		fprintf(stderr, "node_has_leaf: empty node\n");
		return 0;
	}

	/* search if given node has a matching leaf */
	do {
		for (i = 0; i < node->branch_num; i++) {
			if (IS_LEAF(node->next[i]) &&
			    NODE_IS_ALLOWED(node->next[i], view, do_flag)) {
				return 1;
			}
		}
	} while (--max_tries >= 1 && (node = node->alt_head) != NULL);

	return 0;
}

/*
 * check if node is stalk of leaf
 */
static int
node_has_only_leaf(node_t *node, int view, int do_flag)
{
	int	i, branch_cnt = 0, leaf_cnt = 0;

	if (!node) {
		fprintf(stderr, "node_has_only_leaf: empty node\n");
		return 0;
	}

	if (!NODE_IS_ALLOWED(node, view, do_flag))
		return 0;

	for (i = 0; i < node->branch_num; i++) {
		if (NODE_IS_ALLOWED(node->next[i], view, do_flag)) {
			branch_cnt++;
			if (IS_LEAF(node->next[i]))
				leaf_cnt++;
		}
	}
	if (branch_cnt == 1 && leaf_cnt == 1)
		return 1;
	else
		return 0;
}

/*
 * try to get next matching node
 */
static int
get_next_node(node_t *node, node_t **next, char *arg, int view, int do_flag)
{
	int	i, j, res;
	node_t	*first = NULL, *opt = NULL;
	u_char	*mark_candidate = NULL;
	int	max_tries = 2;
	int	n_match = 0;

	if (!node || !arg || !arg[0]) {
		fprintf(stderr, "match_next_node: empty node\n");
		return -1;
	}

	/* for alt younger silbings, always backtrack to eldest */
	if (node->alt_head)
		node = node->alt_head;

	/*
	 * search if given node match a next slot.
	 * 2 tries might be needed when going through option nodes.
	 */
	while (node && max_tries > 0) {

		for (i = 0; i < node->branch_num; i++) {
			/*
			 * opt head matched, dive down to try first layer of option match
			 */
			if (node->next[i]->match_type == MATCH_OPT_HEAD) {
				opt = node->next[i];
				for (j = 0; j < opt->branch_num; j++) {
					if ((res = match_node(opt->next[j], arg, view, do_flag))) {
						if (res == MATCH_EXACTLY) {
							first = opt->next[j];
							n_match = 1;
							opt->opt_mark[j] = 1;
							align_opt_mark(opt, j);
							goto out;
						} else {
							if (first == NULL) {
								first = opt->next[j];
								mark_candidate = &opt->opt_mark[j];
							}
							n_match++;
						}
					}
				}
				continue;
			}

			/* skip parsed branches in opt head node */
			if (node->match_type == MATCH_OPT_HEAD) {
				if (node->opt_mark[i])
					continue;
			}

			if ((res = match_node(node->next[i], arg, view, do_flag))) {
				if (res == MATCH_EXACTLY) {
					first = node->next[i];
					n_match = 1;
					if (node->match_type == MATCH_OPT_HEAD) {
						node->opt_mark[i] = 1;
						align_opt_mark(node, i);
					}
					goto out;
				} else {
					if (first == NULL) {
						first = node->next[i];
						if (node->match_type == MATCH_OPT_HEAD) {
							mark_candidate = &node->opt_mark[i];
						}
					}
					n_match++;
				}
			}
		}

		max_tries--;

		/* for opt end node, backtrack to opt head and try one more loop */
		if (!node->opt_head)
			break;
		else
			node = node->opt_head;
	}

out:
	/* partialy but uniquely matched option, mark used flag */
	if (n_match == 1 && mark_candidate) {
		*mark_candidate = 1;
	}

	*next = first;
	return n_match;
}

/*
 * display debug info of command trees
 */
void
debug_cmd_tree(char *cmd)
{
	struct cmd_tree *ent;
	struct manual *man;
	node_t	*path[MAX_ARG_NUM + 1];
	int	i = 0;

	fprintf(stderr, "cmd_tree = {\n");
	list_for_each_entry(ent, &cmd_tree_list, cmd_tree_list) {
		if (cmd == NULL || strcmp(cmd, ent->cmd) == 0) {
			fprintf(stderr, "[%d] %s\n",i,  ent->cmd);
			list_for_each_entry(man,
					    &ent->manual_list,
					    manual_list) {
				fprintf(stderr, "    %s\n", man->text);
			}
			fprintf(stderr, "    -->\n");
			debug_tree(ent->tree, &path[0], 0);
			fprintf(stderr, "\n");
			if (cmd) break;
		}
		i++;
	}
	fprintf(stderr, "}\n");
}

/*
 * display debug info of one syntax tree node
 */
void
debug_node(char *info, node_t *node, int less)
{
	struct lex_ent *lex;

	if (node == NULL) return;

	if (info && *info) fprintf(stderr, "%s: ", info);
	fprintf(stderr, "node<%p>={", node);

	if (node->match_type == MATCH_KEYWORD) {
		fprintf(stderr, "key:%s=\'%s\',",
			node->arg_name,
			node->match_ent.keyword);
	} else if (node->match_type == MATCH_VAR) {
		lex = get_lex_ent(node->match_ent.var.lex_type);
		fprintf(stderr, "var:%s=%s,",
			node->arg_name,
			lex ? lex->name:"N/A");
		if (!less && node->match_ent.var.chk_range)
			fprintf(stderr, "min=%.2f,max=%.2f,",
				node->match_ent.var.min_val,
				node->match_ent.var.max_val);
	} else if (node->match_type == MATCH_LEAF) {
		fprintf(stderr, "leaf:=<LF>,");
	} else if (node->match_type == MATCH_OPT_HEAD) {
		fprintf(stderr, "opt:=<HEAD>,");
	} else {
		fprintf(stderr, "unknown<%d>,", node->match_type);
	}

	if (!less && node->help[0])
		fprintf(stderr, "help=\'%s\',", node->help);

	if (node->alt_order)
		fprintf(stderr, "alt=%d,", node->alt_order);

	if (node->opt_head)
		fprintf(stderr, "opt=END,");

	fprintf(stderr, "do_view=0x%x,undo_view=0x%x,depth=%d,bnum=%d}\n",
		node->do_view_mask, node->undo_view_mask,
		node->depth, node->branch_num);
}

/*
 * sprout nodes besides each leaf recursively
 */
static void
sprout_tree(node_t *tree, node_t **nodes, int num, int view_mask, int do_flag)
{
	int	i;
	node_t	*base = NULL;

	if (!tree) return;

	/* search if given node has a leaf */
	for (i = 0; i < tree->branch_num; i++) {
		if (IS_LEAF(tree->next[i])) {
			if (base == NULL)
				base = tree;
		} else {
			sprout_tree(tree->next[i], nodes, num,
				    view_mask, do_flag);
		}
	}

	/* only sprout for view_mask matching node */
	if (base != NULL) {
		if ((do_flag & DO_FLAG) && base->do_view_mask != view_mask)
			return;
		if ((do_flag & UNDO_FLAG) && tree->undo_view_mask != view_mask)
			return;
		grow_tree(base, nodes, num, view_mask, do_flag);
	}
}

/*
 * debug tree paths recursively
 */
static void
debug_tree(node_t *tree, node_t **path, int len)
{
	int	i;
	node_t	*node;
	struct lex_ent *lex;

	if (!tree) return;

	if (len < MAX_ARG_NUM) {
		path[len++] = tree;
	}

	if (tree->alt_head) {
		for (i = 0; i < tree->alt_head->branch_num; i++)
			debug_tree(tree->alt_head->next[i], path, len);

	} else if (tree->branch_num == 0) {
		fprintf(stderr, "    ");
		for (i = 0; i < len; i++) {
			if ((node = path[i]) == NULL)
				continue;
			if (node->match_type == MATCH_KEYWORD) {
				fprintf(stderr, "%s ",
					node->match_ent.keyword);
			} else if (node->match_type == MATCH_VAR) {
				lex = get_lex_ent(node->match_ent.var.lex_type);
				fprintf(stderr, "%s ",
					lex ? lex->name:"N/A");
			} else if (node->match_type == MATCH_LEAF)
				fprintf(stderr, "<LF>");

			if (node->match_type == MATCH_OPT_HEAD)
				fprintf(stderr, "[ ");
			else if (node->opt_head)
				fprintf(stderr, "] ");
			else if (node->alt_head && node->alt_head->opt_head)
				fprintf(stderr, "] ");
		}
		fprintf(stderr, "    \n");
	} else {
		for (i = 0; i < tree->branch_num; i++)
			debug_tree(tree->next[i], path, len);
	}
}

/*
 * free tree nodes recursively
 */
static void
free_tree(node_t *tree)
{
	int	i;

	if (!tree) return;
	for (i = 0; i < tree->branch_num; i++)
		free_tree(tree->next[i]);

	if ((debug_flag & DBG_TREE))
		debug_node("free", tree, 1);

	free(tree);
}

/*
 * free command tree
 */
static void
free_cmd_tree(struct cmd_tree *cmd_tree)
{
	dprintf(DBG_TREE, "free tree [%s]\n", cmd_tree->cmd);
	cleanup_manuals(&cmd_tree->manual_list);
	cleanup_symbols(&cmd_tree->symbol_list);
	free_tree(cmd_tree->tree);
	free(cmd_tree);
}

/*
 * set cmd arg from given node and str
 * return 1 if set OK else return 0;
 */
static int
set_cmd_arg(node_t *node, char *str, cmd_arg_t *cmd_arg)
{
	struct lex_ent *lex;

	if (!node || !str || !str[0]) return 0;
	if (!node->arg_name[0] || !cmd_arg) return 0;
	if (node->match_type != MATCH_KEYWORD &&
	    node->match_type != MATCH_VAR)
		return 0;

	cmd_arg->name = strdup(node->arg_name);
	if (node->match_type == MATCH_KEYWORD) {
		cmd_arg->value = strdup(node->match_ent.keyword);
		return 1;
	}

	if ((lex = get_lex_ent(node->match_ent.var.lex_type)) &&
	    lex->fun(str) == 1) {
		cmd_arg->value = strdup(str);
		return 1;
	}
	return 0;
}

/*
 * set auto completion helper for specific argument recursively
 */
void
set_cmd_arg_helper(node_t *tree, char *arg_name, arg_helper_t helper)
{
	int	i;

	if (!tree || !arg_name || !arg_name[0] || !helper) return;

	if (tree->match_type == MATCH_VAR && !tree->arg_helper &&
	    tree->arg_name && tree->arg_name[0] &&
	    strcmp(tree->arg_name, arg_name) == 0) {
		tree->arg_helper = helper;
	}

	for (i = 0; i < tree->branch_num; i++) {
		set_cmd_arg_helper(tree->next[i], arg_name, helper);
	}
}

/*
 * debug content of command arg array
 */
void
debug_cmd_arg(cmd_arg_t *cmd_arg)
{
	int	i;

	if (!cmd_arg) return;
	for (i = 0; i < MAX_ARG_NUM; i++) {
		if (cmd_arg[i].name == NULL && cmd_arg[i].value == NULL)
			break;
		fprintf(stderr, "  cmd_arg[%d]={\'%s\':\'%s\'}\n",
			i, cmd_arg[i].name, cmd_arg[i].value);
	}
}

/*
 * free command arg array
 */
void
free_cmd_arg(cmd_arg_t *cmd_arg)
{
	int	i;

	if (!cmd_arg) return;
	for (i = 0; i < MAX_ARG_NUM; i++) {
		if (cmd_arg[i].name) free(cmd_arg[i].name);
		if (cmd_arg[i].value) free(cmd_arg[i].value);
	}
	free(cmd_arg);
}

/*
 * printf the syntax error info by cmd_stat
 */
void
perror_cmd_stat(char *prompt, struct cmd_stat *cmd_stat)
{
	if (cmd_stat->err_code == 0) return;
	
	if (prompt && prompt[0])
		fprintf(stdout, "%s", prompt);
	if (cmd_stat->err_argi >= 0 && cmd_stat->err_arg) {
		fprintf(stdout, "Parsing error at word[%d] \'%s\': %s\n",
			cmd_stat->err_argi + 1, cmd_stat->err_arg,
			ocli_strerror(cmd_stat->err_code));
	} else {
		fprintf(stdout, "%s\n", ocli_strerror(cmd_stat->err_code));
	}
}

/*
 * debug cmd_stat updated by check_cmd_syntax()
 */
void
debug_cmd_stat(struct cmd_stat *cmd_stat)
{
	if (!cmd_stat) return;
	fprintf(stderr, "  cmd=\'%s\'\n",
		cmd_stat->cmd_tree ? cmd_stat->cmd_tree->cmd:"Not found");
	fprintf(stderr, "  err=\'%s\',err_arg[%d]=\'%s\',offset=%d\n",
		ocli_strerror(cmd_stat->err_code),
		cmd_stat->err_argi,
		cmd_stat->err_arg ? cmd_stat->err_arg:"NULL",
		cmd_stat->err_offset);
	fprintf(stderr, "  last_arg[%d]=\'%s\',offset=%d,",
		cmd_stat->last_argi,
		cmd_stat->last_arg ? cmd_stat->last_arg:"NULL",
		cmd_stat->last_offset);
	if (cmd_stat->last_node)
		debug_node("last_node", cmd_stat->last_node, 1);
	else
		fprintf(stderr, "last_node=NULL\n");
}

/*
 * cleanup allocated memory of cmd_stat structure
 */
void
cleanup_cmd_stat(struct cmd_stat *cmd_stat)
{
	if (!cmd_stat) return;
	if (cmd_stat->last_arg) free(cmd_stat->last_arg);
	if (cmd_stat->err_arg) free(cmd_stat->err_arg);
	if (cmd_stat->cmd_arg) free_cmd_arg(cmd_stat->cmd_arg);
}

/*
 * get err string by error code
 */
char *
ocli_strerror(int err_code)
{
	if (err_code >= 0 && err_code < MAX_ERROR_CODE)
		return err_info[err_code];
	else
		return err_info[MAX_ERROR_CODE];
}

/*
 * set debug flag
 */
void
ocli_set_debug(int flag)
{
	debug_flag = flag;
}

/*
 * init core module
 */
int
ocli_core_init(void)
{
	if (olic_core_init_ok) return 0;

	lex_init();
	symbol_init();
	INIT_LIST_HEAD(&cmd_tree_list);

	olic_core_init_ok = 1;
	return 0;

}

/*
 * exit core module
 */
void
ocli_core_exit(void)
{
	struct cmd_tree *ent, *tmp;

	list_for_each_entry_safe(ent, tmp, &cmd_tree_list, cmd_tree_list) {
		free_cmd_tree(ent);
	}

	symbol_exit();
	lex_exit();
	olic_core_init_ok = 0;
}
