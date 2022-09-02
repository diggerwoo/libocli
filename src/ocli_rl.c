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
 * ocli_rl.c, the readline interface module of libocli
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>

#include "ocli.h"

#define TERM_TIMO_SEC	180
#define HELP_BUF_SIZE	4096

int ocli_rl_finished = 0;

static int ocli_rl_timeout_flag = 0;

#define	DBG_RL		0x01

static int debug_flag = 0;

static int cur_view = BASIC_VIEW;
static char cur_prompt[MAX_WORD_LEN];
static char word_break_chars[] = " \t\r\n";

static char eof_cmd[MAX_WORD_LEN] = "";

/* limit max helping tokens displayed */
#define MAX_TOK_NUM	80
static char *pending_toks[MAX_TOK_NUM+1];
static int toks_index = 0;

static struct termios init_termios;

static int term_timo = TERM_TIMO_SEC;

static void free_pending_toks(void);

/* local callback functions for readline completion */
static char *ocli_rl_generator(const char *text, int state);
static char **ocli_rl_prepare(char *text, int start, int end);
static int ocli_rl_help(int count, int key);
static char *ocli_rl_set_word_break(void);

/*
 * completion generator
 */
static char *
ocli_rl_generator(const char *text, int state)
{
	static int len;
	char	*tok = NULL;

	/* First call */
	if (!state) {
		toks_index = 0;
		len = strlen(text);
	}

	/* Return the tokens which partially matches from pending_toks[] */
	while (toks_index < MAX_TOK_NUM && (tok = pending_toks[toks_index])) {
		toks_index++;
		if (strncmp(tok, text, len) == 0) {
			dprintf(DBG_RL, "gen stat=%d tok=\'%s\'\n", state, tok);
			return (strdup(tok));
		}
	}

	/* If no names matched, then return NULL. */
	dprintf(DBG_RL, "gen stat=%d tok=NULL\n", state);
	return NULL;
}

/*
 * preparation for readline completion
 */
static char **
ocli_rl_prepare(char *text, int start, int end)
{
	int	i, arg_end;
	int	arg_num = 0;
	char	**args = NULL;
	int	starts[MAX_ARG_NUM];
	int	argi = -1;
	int	ignore = 0;
	int	tok_num = 0;
	int	res;
	char	*cmd = NULL;
	struct cmd_stat cmd_stat;

	/* free used pending_toks for each preparation */
	free_pending_toks();

	bzero(&cmd_stat, sizeof(cmd_stat));
	bzero(&starts[0], sizeof(starts));
	arg_num = get_argv(rl_line_buffer, &args, &starts[0]);

	for (i = 0; i < arg_num; i++) {
		arg_end = starts[i] + strlen(args[i]) - 1;
		/* Ignore completion if rl_point inside or before word */
		if (rl_point >= starts[i] && rl_point <= arg_end) {
			dprintf(DBG_RL, "inside arg[%d], ignore\n", i);
			ignore = 1;
			break;
		} else if (rl_point < starts[i]) {
			dprintf(DBG_RL, "begore arg[%d], ignore\n", i);
			ignore = 1;
			break;
		} else if (rl_point == (arg_end + 1)) {
			dprintf(DBG_RL, "right after arg[%d], OK\n", i);
			argi = i;
			break;
		}
	}

	if (ignore) goto out;

	dprintf(DBG_RL, "-->[%s], pos[%d], ", rl_line_buffer, rl_point);
	dprintf(DBG_RL, "complete: arg[%d]=[%s]\n", argi, text);
	
	if (arg_num == 0) {
		tok_num = get_node_matches(NULL, NULL, &pending_toks[0],
					   MAX_TOK_NUM, cur_view, DO_FLAG);
		goto out;
	}

	if ((cmd = strndup(rl_line_buffer, rl_point)) == NULL) {
		fprintf(stderr, "ocli_rl_prepare: no memory for strdup\n");
		ignore = 1;
		goto out;
	}

	res = check_cmd_syntax(cmd, cur_view, &cmd_stat);

	if ((debug_flag & 1)) debug_cmd_stat(&cmd_stat);

	if (cmd_stat.err_code != MATCH_OK &&
	    cmd_stat.err_code != MATCH_ERROR &&
	    cmd_stat.err_code != MATCH_AMBIGUOUS &&
	    cmd_stat.err_code != MATCH_INCOMPLETE) {
		ignore = 1;
		goto out;
	}

	if (cmd_stat.last_argi == argi) {
		dprintf(DBG_RL, "res %d,last[%d]=argi[%d]\n",
			res, cmd_stat.last_argi, argi);
		tok_num = get_node_matches(cmd_stat.last_node, text,
					   &pending_toks[0], MAX_TOK_NUM,
					   cur_view, cmd_stat.do_flag);
	} else if (cmd_stat.last_node != NULL &&
		   cmd_stat.last_argi == (argi - 1)) {
		dprintf(DBG_RL, "res %d,last[%d]=argi[%d]-1\n",
			res, cmd_stat.last_argi, argi);
		tok_num = get_node_next_matches(cmd_stat.last_node, text,
						&pending_toks[0], MAX_TOK_NUM,
						cur_view, cmd_stat.do_flag);
	} else if (cmd_stat.last_node != NULL &&
		   cmd_stat.last_argi == (arg_num - 1) && argi == -1) {
		dprintf(DBG_RL, "res %d, after last[%d]\n",
			res, cmd_stat.last_argi);
		tok_num = get_node_next_matches(cmd_stat.last_node, NULL,
						&pending_toks[0], MAX_TOK_NUM,
						cur_view, cmd_stat.do_flag);
	} else {
		dprintf(DBG_RL, "NULL, res %d last[%d] argi[%d]\n",
			res, cmd_stat.last_argi, argi);
		ignore = 1;
	}
out:
	if (cmd != NULL) free(cmd);
	if (args != NULL) free_argv(args);

	if (!ignore && tok_num > 0) {
		cleanup_cmd_stat(&cmd_stat);
		return rl_completion_matches(text, ocli_rl_generator);
	} else {
		if (cmd_stat.err_code == MATCH_ERROR && tok_num == 0) {
			rl_crlf();
			perror_cmd_stat(NULL, &cmd_stat);
			rl_on_new_line();
		}
		cleanup_cmd_stat(&cmd_stat);
		return NULL;
	}
}

/*
 * help func for stroking '?'
 */
static int
ocli_rl_help(int count, int key)
{
	int	arg_end;
	int	arg_num = 0;
	char	**args = NULL;
	int	starts[MAX_ARG_NUM];
	int	argi = -1;
	int	res, len = 0;
	char	*help_buf = NULL;
	char	*cmd = NULL;
	struct cmd_stat cmd_stat;

	/* tricky to rewrite '\?' as '?' */
	if (rl_end > 0 && rl_end == rl_point && rl_line_buffer[rl_end-1] == '\\') {
		rl_line_buffer[rl_end-1] = '?';
		rl_redisplay();
		return 0;
	}

	bzero(&cmd_stat, sizeof(cmd_stat));
	bzero(&starts[0], sizeof(starts));
	arg_num = get_argv(rl_line_buffer, &args, &starts[0]);

	if (arg_num > 0) {
		arg_end = starts[arg_num-1] + strlen(args[arg_num-1]) - 1;
		if (rl_point == (arg_end + 1)) {
			dprintf(DBG_RL, "\nhelp the last word\n");
			argi = arg_num - 1;
		} else if (rl_point > (arg_end + 1)) {
			dprintf(DBG_RL, "\nhelp after the last word\n");
		} else {
			dprintf(DBG_RL, "ignore help\n");
			free_argv(args);
			return 0;
		}
	}

	rl_crlf();
	if ((help_buf = malloc(HELP_BUF_SIZE)) == NULL) {
		fprintf(stderr, "no memory for help buf\n");
		goto out;
	}
	bzero(help_buf, HELP_BUF_SIZE);
	if (arg_num == 0) {
		dprintf(DBG_RL, "first help\n");
		get_node_help(NULL, NULL, help_buf, HELP_BUF_SIZE,
			      cur_view, DO_FLAG);
		goto out;
	}

	res = check_cmd_syntax(rl_line_buffer, cur_view, &cmd_stat);

	if ((debug_flag & 1)) debug_cmd_stat(&cmd_stat);

	if (cmd_stat.err_code != MATCH_OK &&
	    cmd_stat.err_code != MATCH_ERROR &&
	    cmd_stat.err_code != MATCH_AMBIGUOUS &&
	    cmd_stat.err_code != MATCH_INCOMPLETE) {
		perror_cmd_stat(NULL, &cmd_stat);
		goto out;
	}

	if (cmd_stat.last_argi == argi) {
		dprintf(DBG_RL, "res %d,last[%d]=argi[%d]\n",
			res, cmd_stat.last_argi, argi);
		len = get_node_help(cmd_stat.last_node, args[argi],
				    help_buf, HELP_BUF_SIZE,
				    cur_view, cmd_stat.do_flag);
	} else if (cmd_stat.last_node != NULL &&
		   cmd_stat.last_argi == (argi - 1)) {
		dprintf(DBG_RL, "res %d,last[%d]=argi[%d]-1\n",
			res, cmd_stat.last_argi, argi);
		len = get_node_next_help(cmd_stat.last_node, args[argi],
					 help_buf, HELP_BUF_SIZE,
					 cur_view, cmd_stat.do_flag);
	} else if (cmd_stat.last_node != NULL &&
		   cmd_stat.last_argi == (arg_num - 1) && argi == -1) {
		dprintf(DBG_RL, "res %d, after last[%d]\n",
			res, cmd_stat.last_argi);
		len = get_node_next_help(cmd_stat.last_node, NULL,
					 help_buf, HELP_BUF_SIZE,
					 cur_view, cmd_stat.do_flag);
	} else {
		dprintf(DBG_RL, "NULL, res %d last[%d] argi[%d]\n",
			res, cmd_stat.last_argi, argi);
	}

	if ((cmd_stat.err_code == MATCH_ERROR ||
	     cmd_stat.err_code == MATCH_AMBIGUOUS) && len == 0) {
		perror_cmd_stat(NULL, &cmd_stat);
	}
out:
	if (cmd != NULL) free(cmd);
	if (args != NULL) free_argv(args);
	cleanup_cmd_stat(&cmd_stat);

	if (help_buf[0]) display_buf_more(help_buf, HELP_BUF_SIZE);
	if (help_buf) free(help_buf);

	rl_on_new_line();
	return 0;
}

/*
 * submit command
 */
int
ocli_rl_submit(char *cmd, int view)
{
	int	res;
	struct cmd_stat cmd_stat;

	/* XXX NO submit the last timed-out command */
	if (ocli_rl_finished) return 0;

	bzero(&cmd_stat, sizeof(cmd_stat));
	res = check_cmd_syntax(cmd, view, &cmd_stat);

	if (debug_flag) debug_cmd_stat(&cmd_stat);

	/* syntax parsing OK, call cmd_tree->fun with cmd_arg ... */
	if (res == 0) {
		if (cmd_stat.cmd_tree->fun) {
			cmd_stat.cmd_tree->fun(cmd_stat.cmd_arg,
					       cmd_stat.do_flag);
		}
	} else {
		perror_cmd_stat(NULL, &cmd_stat);
	}
	cleanup_cmd_stat(&cmd_stat);
	return res;
}

/*
 * set auto completion enabled or disabled
 */
int
ocli_rl_set_auto_completion(int enabled)
{
	int	res;

	/* Always simpify the word break by white spaces */
	rl_completion_word_break_hook = ocli_rl_set_word_break;

	if (enabled) {
		/* Tell the completer that we want to prepare first. */
		rl_attempted_completion_function =
			(rl_completion_func_t *) ocli_rl_prepare;
		rl_completion_entry_function =
			(rl_compentry_func_t *) ocli_rl_generator;
			
		res = rl_bind_key('\t', rl_complete);
		res = rl_bind_key('?', (rl_command_func_t *) ocli_rl_help);
	} else {
		res = rl_bind_key('\t', rl_abort);
		res = rl_bind_key('?', rl_insert);
	}
	return res;
}

/*
 * set default word break chars
 */
static char *
ocli_rl_set_word_break()
{
	return &word_break_chars[0];
}

/*
 * set current view
 */
void
ocli_rl_set_view(int view)
{
	cur_view = view;
}

/*
 * get current view
 */
int
ocli_rl_get_view()
{
	return (cur_view);
}

/*
 * set current readline prompt
 */
void
ocli_rl_set_prompt(char *prompt)
{
	if (prompt && *prompt)
		snprintf(cur_prompt, sizeof(cur_prompt), prompt);
}

/*
 * set readline timeout
 */
void
ocli_rl_set_timeout(int sec)
{
	term_timo = sec;
}

/*
 * set echo on or off
 */
int
ocli_rl_set_echo(int on)
{
	struct termios cur_termios;

	tcgetattr(0, &cur_termios);

	if (on)
		cur_termios.c_lflag |= (ECHO|ECHOE|ECHOK|ECHONL);
	else
		cur_termios.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL);

	return tcsetattr(0, TCSADRAIN, &cur_termios);
}

/*
 * a readline wrapper function witht auto completion disabled
 */
char *
read_bare_line(char *prompt)
{
	char	*line = NULL;

	ocli_rl_set_auto_completion(0);
	line = readline(prompt);
	ocli_rl_set_auto_completion(1);
	return (line);
}
/*
 * a readline wrapper function for reading a password
 */
char *
read_password(char *prompt)
{
	char	*password = NULL;

	ocli_rl_set_echo(0);
	ocli_rl_set_auto_completion(0);

	password = readline(prompt);

	ocli_rl_set_echo(1);
	ocli_rl_set_auto_completion(1);
	rl_crlf();
	return (password);
}

/*
 * clean pending toks
 */
static void
free_pending_toks()
{
	int	i;

	if (pending_toks[0]) {
		for (i = 0; i < (MAX_TOK_NUM+1) && pending_toks[0]; i++)
			free(pending_toks[i]);
		bzero(&pending_toks[0], sizeof(pending_toks));
	}
	toks_index = 0;
}

/*
 * customized getc using select with timeout
 */
int
ocli_rl_getc(FILE *fp)
{
	struct timeval timeval, *val;
	fd_set	fdr;
	int	res;

	FD_ZERO(&fdr);
	FD_SET(fileno(fp), &fdr);

	if (term_timo > 0) {
		timeval.tv_sec = term_timo;
		timeval.tv_usec = 0;
		val = &timeval;
	} else {
		val = NULL;
	}

	ocli_rl_timeout_flag = 0;
	res = select(fileno(fp)+1, &fdr, NULL, NULL, val);

	if (term_timo > 0 && res == 0) {
		ocli_rl_timeout_flag = 1;
		ocli_rl_finished = 1;
		return EOF;
	}

	return rl_getc(fp);
}

/*
 * SIG_TERM handler
 */
static void
ocli_sig_term(sig)
int sig;
{
	fprintf(stdout, "\nTerminated by signal %d\n", sig);
	ocli_rl_exit();
	exit(1);
}

/*
 * set  debug flag
 */
void
ocli_rl_set_debug(int flag)
{
	debug_flag = flag;
}

/*
 * set command to be trigger when EOF encountered
 */
void
ocli_rl_set_eof_cmd(char *cmd)
{
	if (cmd && cmd[0])
		snprintf(eof_cmd, sizeof(eof_cmd), cmd);
}

/*
 * exec EOF command
 */
void
ocli_rl_exec_eof_cmd()
{
	if (eof_cmd[0]) {
		rl_insert_text(eof_cmd);
		rl_redisplay();
		rl_crlf();
		ocli_rl_submit(eof_cmd, ocli_rl_get_view());
	}
}

/*
 * readline loop with auto completion enabled
 */
void
ocli_rl_loop()
{
	char	*cmd;

	ocli_rl_set_auto_completion(1);

	while (!ocli_rl_finished) {
		if ((cmd = readline(cur_prompt))) {
			if (!is_empty_line(cmd)) {
				ocli_rl_submit(cmd, ocli_rl_get_view());
				add_history(cmd);
			}
		} else {
			ocli_rl_exec_eof_cmd();
		}
	}
}

/*
 * exit readline interface module
 */
void
ocli_rl_exit()
{
	if (ocli_rl_timeout_flag)
		fprintf(stdout, "\nTimeout, abort\n");

	free_pending_toks();
	tcsetattr(0, TCSADRAIN, &init_termios);

	ocli_core_exit();
}

/*
 * init readline interface module
 */
int
ocli_rl_init()
{
	ocli_core_init();

	bzero(cur_prompt, MAX_WORD_LEN);
	bzero(&pending_toks[0], sizeof(pending_toks));
	toks_index = 0;

	tcgetattr(0, &init_termios);
	rl_getc_function = (rl_getc_func_t *) ocli_rl_getc;

	/*
	 * readline() will handle SIGINT, SIGQUIT, SIGTERM, SIGALRM,
	 * SIGTSTP, SIGTTIN, SIGTTOU, and SIGWINCH itself, then call
	 * customized upper layer handler, finally go back to its IO
	 * loop again.
	 * XXX upgly setjmp/longjmp might be needed for specific
	 * customized signal handler.
	 */
	if (!debug_flag) signal(SIGINT, SIG_IGN);
	signal(SIGTERM, ocli_sig_term);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	return 0;
}
