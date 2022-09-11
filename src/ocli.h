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

#ifndef	_OCLI_H
#define	_OCLI_H

#include <stdio.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "list.h"
#include "ocli_defs.h"
#include "lex.h"

/* parsing error code definitions */
enum error_code {
	MATCH_OK,
	MATCH_ERROR,
	MATCH_AMBIGUOUS,
	MATCH_INCOMPLETE,
	TOO_MANY_ARGS,
	MAX_ERROR_CODE
};

/*
 * A syntax tree node definition
 */

/* node types */
#define	IS_ROOT(n)	(n->depth == 0)
#define	IS_LEAF(n)	(n->match_type == MATCH_LEAF && n->branch_num == 0)

/* do flag bits */
#define	DO_FLAG		0x01
#define	UNDO_FLAG	0x02

/* match type */
#define MATCH_KEYWORD	1	/* match a constant keyword */
#define	MATCH_VAR	2	/* match a variable */
#define	MATCH_LEAF	3	/* match a LeaF, a Line Feed */

#define	MATCH_OPT_HEAD	4	/* match a pseudo opt head node */
#define	MATCH_OPT_END	5	/* match a pseudo opt end node */
#define	MATCH_OPT_ANY	6	/* match a pseudo opt any node */

#define	MATCH_ALT_HEAD	7	/* match a pseudo alt head node */
#define	MATCH_ALT_END	8	/* match a pseudo alt end node */
#define	MATCH_ALT_OR	9	/* match a pseudo alt or node */

#define MATCH_EXACTLY	100	/* keyword matched exactly */

/* related limits of OCLI strings */
#define	MAX_BRANCH_NUM	80	/* max branch per node */
#define	MAX_WORD_LEN	32	/* max key word length */
#define	MAX_TEXT_LEN	128	/* max text length */
#define	MAX_MANUAL_LEN	256	/* max manual text length */
#define	MAX_LINE_LEN	512	/* max command line length */
#define	MAX_ARG_NUM	50	/* max arg num per command */
#define	MAX_CMD_NUM	100	/* max commands */

#define MAX_CHOICES	16	/* limit of choices for [] {} */

typedef struct var {
	int	lex_type;	/* lexical type */
	int	chk_range;	/* if enabled numeric range check */
	double	min_val;	/* minimal value of range */
	double	max_val;	/* maximal value of range */
} var_t;

typedef struct node node_t;

#define NODE_IS_ALLOWED(node, view, do_flag) \
	(((do_flag & DO_FLAG) && (view & node->do_view_mask) != 0) || \
	 ((do_flag & UNDO_FLAG) && (view & node->undo_view_mask) != 0))

typedef int (*arg_helper_t)(char *, char **, int);

struct node {
	int	match_type;		/* keyword or variable */
	union {
		char	keyword[MAX_WORD_LEN];	/* the keyword string */
		var_t	var;			/* the variable item */
	} match_ent;

	u_int	do_view_mask;		/* the do view mask */
	u_int	undo_view_mask;		/* the undo view mask */
	char	arg_name[MAX_WORD_LEN];	/* arg name for command exec */
	char	help[MAX_TEXT_LEN];	/* help text info */

	arg_helper_t arg_helper;	/* helper func for auto completion */

	int	depth;			/* tree node depth, 0 is root */
	int	branch_num;		/* number of branches */
	node_t	*next[MAX_BRANCH_NUM];	/* array of branch pointers */

	u_char	opt_mark[MAX_BRANCH_NUM];	/* opt head: branch used mark */
	node_t	*opt_head;			/* opt end: backtrack to opt head */

	int	alt_order;		/* alt silbing order: eldest = 1 */
	node_t	*alt_head;		/* alt younger siblings, backtrack to eldest */
};

#define	MANUAL_ARG	"_CMD_"	/* tricky for manual node->arg_name */

/* manual entry */
struct manual {
	char	text[MAX_MANUAL_LEN];	/* the manual text */
	u_int	view_mask;		/* the view mask of manual */
	struct list_head manual_list;	/* link to list of manuals */
};

/* Definition of symbol type */
typedef struct symbol {
	char	*name;		/* name of symbol */
	char	*help;		/* symbol help info */
	int	lex_type;	/* if a lexcial type is set, -1 N/A */
	int	chk_range;	/* if enable numeric range check */
	double	min_val;	/* minimal value of range */
	double	max_val;	/* maximal value of range */
	char	*arg_name;	/* if set an arg name */
	node_t	*node;		/* pointer to an assiciated node */
	struct list_head list;	/* link to symbol table */
} symbol_t;

/* Macros DEF_XXX to simply symbol element initialization
 *	n	- name string of symbol
 *	h	- help string of symbol
 *	t	- lexical type ID, e.g. LEX_INT
 *	c	- if range check enabled, 1 or 0
 *	x	- min vallue of range
 *	y	- max vallue of range
 *	a	- callback arg name, usually by ARG() macro
 */

#define DEF_SYM(n, h, t, c, x, y, a) {	\
	.name		= n,		\
	.help		= h,		\
	.lex_type	= t,		\
	.chk_range	= c,		\
	.min_val	= x,		\
	.max_val	= y,		\
	.arg_name	= a,		\
	.node		= NULL,		\
	.list		= {NULL, NULL}	\
}

/* Define a keyword symbol */
#define DEF_KEY(n, h) \
	DEF_SYM(n, h, -1, 0, 0, 0, NULL)

/* Define a keyword symbol with arg */
#define DEF_KEY_ARG(n, h, a) \
	DEF_SYM(n, h, -1, 0, 0, 0, a)

/* Define a var symbol with type and arg */
#define DEF_VAR(n, h, t, a) \
	DEF_SYM(n, h, t, 0, 0, 0, a)

/* Define a var symbol with type, arg and range */
#define DEF_VAR_RANGE(n, h, t, a, x, y) \
	DEF_SYM(n, h, t, 1, x, y, a)

/* Define a reserved syntax symbol */
#define DEF_RSV(n, h) \
	DEF_SYM(n, h, -2, 0, 0, 0, NULL)

/* Definition of arg type of name/value pair */
typedef struct cmd_arg {
	char	*name;		/* arg name */
	char	*value;		/* arg value */
} cmd_arg_t;

/* ARG(X) - implicitly declare a local macro X with string value "X" */
#define	ARG(x)		#x

/* IS_ARG(name, X) - check if *name equals to the string defined by macro X */
#define	STRINGIFY(x)	#x
#define	ARG2(x)		STRINGIFY(x)
#define	IS_ARG(n, x)	(strcmp(n, ARG2(x)) == 0)

/*
 * for_each_cmd_arg - iterate each name/value pair in cmd_arg array
 * @cmd_arg:	the cmd_arg_t pointer to array 
 * @i:		the index int var
 * @name:	the name char pointer to each cmd_arg
 * @value:	the value char pointer of each cmd_arg
 *
 * predefine the following vars before calling this MACRO
 * 	int i;
 * 	char *name, *value;
 */
#define for_each_cmd_arg(cmd_arg, i, name, value)	\
	 for (i = 0; \
	      i < MAX_ARG_NUM && (name = cmd_arg[i].name) && \
		(value = cmd_arg[i].value); i++)

/* Command parsing result status set by check_cmd_syntax() */
typedef struct cmd_stat {
	int	do_flag;	/* do or undo flag */
	int	err_code;	/* error code */
	char	*last_arg;	/* the last parsing arg */
	int	last_argi;	/* index of last_arg */
	int	last_offset;	/* char offset of last_arg */
	node_t	*last_node;	/* the last matching node */
	char	*err_arg;	/* the failed arg */
	int	err_argi;	/* index of err_arg */
	int	err_offset;	/* char offset of err_arg */
	struct cmd_tree *cmd_tree;	/* matching cmd_tree */
	cmd_arg_t *cmd_arg;		/* set cmd_arg */
} cmd_stat_t;

/* Definition of command exec function type */
typedef int (*cmd_fun_t)(cmd_arg_t *, int);

/* a command tree, one tree, multi manuals ... */
struct cmd_tree {
	char	cmd[MAX_WORD_LEN];	/* command name */
	node_t	*tree;			/* root of syntax tree */
	cmd_fun_t fun;			/* command exec function */
	struct list_head symbol_list;	/* list head of symbols */
	struct list_head manual_list;	/* list head of manuals */
	struct list_head cmd_tree_list;	/* link to list of command tree */
};
	
/* declare module static debug_flag to call this */
#define	dprintf(x, ...) \
	if ((debug_flag & x)) fprintf(stderr, __VA_ARGS__)

/*
 * symbol utils functions
 */
extern int set_symbol_node(symbol_t *symbol);
extern int prepare_symbols(struct list_head *sym_list,
			   symbol_t *sym_table, int limit);
extern void cleanup_symbols(struct list_head *sym_list);
extern symbol_t *get_symbol_by_name(struct list_head *sym_list, char *name);
extern node_t *get_node_by_name(struct list_head *sym_list, char *name);
extern int symbol_init(void);
extern void symbol_exit(void);

/*
 * argv utils functions
 */
extern int get_argv(char *str, char ***argvp, int *offsets);
extern void debug_argv(char **argv);
extern void free_argv(char **argv);

/*
 * 'more' utils functions
 */
extern int display_buf_more(char *buf, int total);
extern int display_file_more(char *path);

/* Macros to simplify create_cmd_tree() calls:
 *
 *  - if syms is declared as an array of symbol_t:
 *    {
 *      symbol_t syms[] = { ... } ;
 *      create_cmd_tree(cmd, SYM_TABLE(syms), fun);
 *    }
 *
 *  - if sym is declared as a single symbol_t:
 *    {
 *      symbol_t syms;
 *      create_cmd_tree(cmd, SYM_ROW(sym), fun);
 *    }
 *
 */ 
#define SYM_NUM(syms) (sizeof(syms)/sizeof(symbol_t))
#define SYM_TABLE(syms) &syms[0], SYM_NUM(syms)
#define SYM_ROW(sym) &sym, 1

/* Macros to simplify dynamic command creation and symbol registration
 * without a predefined symbol table.
 *
 * parameters:
 *	pct	- result pointer to (struct cmd_tree *)
 *	n	- name string of symbol
 *	h	- help string of symbol
 *	t	- lexical type ID, e.g. LEX_INT
 *	c	- if range check enabled, 1 or 0
 *	x	- min vallue of range
 *	y	- max vallue of range
 *	a	- callback arg name, usually by ARG() macro
 *	f	- callback function
 */

/* create a command */
#define create_cmd(pct, n, h, f)		\
{						\
	symbol_t s;				\
	s = (symbol_t) DEF_KEY(n, h);		\
	*pct = create_cmd_tree(n, SYM_ROW(s), f); \
}

/* create a command, with the first key has an callback argument */
#define create_cmd_arg(pct, n, h, a, f)		\
{						\
	symbol_t s;				\
	s = (symbol_t) DEF_KEY_ARG(n, h, a);	\
	*pct = create_cmd_tree(n, SYM_ROW(s), f); \
}

/* add a command keyword */
#define add_cmd_key(ct, n, h) 			\
{						\
	symbol_t s;				\
	s = (symbol_t) DEF_KEY_ARG(n, h);	\
	add_cmd_symbol(ct, &s);			\
}

/* add a command keyword with callback argument */
#define add_cmd_key_arg(ct, n, h, a) 		\
{						\
	symbol_t s;				\
	s = (symbol_t) DEF_KEY_ARG(n, h, a);	\
	add_cmd_symbol(ct, &s);			\
}

/* add a command var */
#define add_cmd_var(ct, n, h, t, a) 		\
{						\
	symbol_t s;				\
	s = (symbol_t) DEF_VAR(n, h, t, a);	\
	add_cmd_symbol(ct, &s);			\
}

/* add a command var with ranged check */
#define add_cmd_var_range(ct, n, h, t, a, x, y)		\
{							\
	symbol_t s;					\
	s = (symbol_t) DEF_VAR_RANGE(n, h, t, a, x, y);	\
	add_cmd_symbol(ct, &s);				\
}

/*
 * core functions
 */
extern struct cmd_tree *create_cmd_tree(char *cmd, symbol_t *sym_table, int sym_num,
					cmd_fun_t fun);
extern int get_cmd_tree(char *cmd, int view, int do_flag,
			struct cmd_tree **cmd_tree);
extern int add_cmd_manual(struct cmd_tree *cmd_tree, char *text, int view_mask);
extern int get_cmd_manual(struct cmd_tree *cmd_tree, int view,
			  char *buf, int limit);
extern int add_cmd_symbol(struct cmd_tree *cmd_tree, symbol_t *sym);
extern int add_cmd_syntax(struct cmd_tree *cmd_tree, char *syntax,
			  int view_mask, int do_flag);
extern int add_cmd_easily(struct cmd_tree *cmd_tree, char *syntax,
			  int view_mask, int do_flag);
extern int sprout_cmd_syntax(struct cmd_tree *cmd_tree, char *syntax,
			     int view_mask, int do_flag);
extern int check_cmd_syntax(char *cmd_str, int view, cmd_stat_t *cmd_stat);

extern int get_node_matches(node_t *node, char *cmd, char **matches,
			    int limit, int view, int do_flag);
extern int get_node_next_matches(node_t *node, char *cmd, char **matches,
				 int limit, int view, int do_flag);
extern int get_node_help(node_t *node, char *cmd, char *buf, int limit,
			 int view, int do_flag);
extern int get_node_next_help(node_t *node, char *cmd, char *buf, int limit,
			      int view, int do_flag);
extern int compare_node(node_t *node1, node_t *node2);

extern void debug_cmd_tree(char *cmd);
extern void debug_node(char *info, node_t *node, int less);

extern void set_cmd_arg_helper(node_t *tree, char *arg_name, arg_helper_t helper);
extern void debug_cmd_arg(cmd_arg_t *cmd_arg);
extern void free_cmd_arg(cmd_arg_t *cmd_arg);

extern void perror_cmd_stat(char *prompt, struct cmd_stat *cmd_stat);
extern void debug_cmd_stat(struct cmd_stat *cmd_stat);
extern void cleanup_cmd_stat(struct cmd_stat *cmd_stat);

extern char *ocli_strerror(int err_code);
extern void ocli_set_debug(int flag);

extern int ocli_core_init(void);
extern void ocli_core_exit(void);

/*
 * readline interface vars and functions
 */
extern int ocli_rl_finished;

extern char *read_bare_line(char *prompt);
extern char *read_password(char *prompt);
extern int ocli_rl_submit(char *cmd, int view);
extern int ocli_rl_set_auto_completion(int enabled);
extern void ocli_rl_set_timeout(int sec);
extern int ocli_rl_set_echo(int on);
extern int ocli_rl_get_view(void);
extern void ocli_rl_set_view(int view);
extern void ocli_rl_set_prompt(char *prompt);

extern int ocli_rl_getc(FILE *fp);
extern void ocli_rl_set_debug(int flag);

extern void ocli_rl_set_eof_cmd(char *cmd);
extern void ocli_rl_exec_eof_cmd(void);

extern void ocli_rl_loop(void);

extern int ocli_rl_init(void);
extern void ocli_rl_exit(void);

/*
 * bonus built-in commands
 */
int cmd_undo_init(void);
int cmd_manual_init(void);

#endif	/* _OCLI_H */
