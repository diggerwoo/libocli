/*
 *
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
 * utils.c, a misc utils module
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <readline/readline.h>

#include "lex.h"
#include "ocli.h"

/*
 * split str into arg vector, support "words..." arg format
 * XXX for sure the content of str will not be modified 
 * return number of args and set *argvp to pointer array
 * set each arg offset into offsets if offsets is not NULL
 */ 
int
get_argv(char *str, char ***argvp, int *offsets)
{
	char	*ptr, tok[MAX_TEXT_LEN];
	int	arg_num, tok_offset, tok_pos, len;
	int	in_space, in_quota;

	if (!str || !str[0]) {
		*argvp = NULL;
		return 0;
	}
	
	if ((*argvp = malloc((MAX_ARG_NUM+1) * sizeof(char *))) == NULL) {
		fprintf(stderr, "malloc argvp: %s", strerror(errno));
		*argvp = NULL;
		return -1;
	}
	bzero(*argvp, (MAX_ARG_NUM+1) * sizeof(char *));
	if (offsets) bzero(offsets, MAX_ARG_NUM * sizeof(int));

	ptr = str;
	bzero(tok, MAX_TEXT_LEN);
	tok_pos = 0;
	tok_offset = 0;
	len = 0;

	arg_num = 0;
	in_space = 1;
	in_quota = 0;

	while (*ptr && len < MAX_LINE_LEN && arg_num < MAX_ARG_NUM) {
		if (in_quota == 1) {
			if (*ptr != '\"') {
				if (tok_pos == 0) tok_offset = ptr - str;
				if (tok_pos < (MAX_TEXT_LEN-1))
					tok[tok_pos++] = *ptr;
			} else {
				in_quota = 2;
			}
		} else if (in_quota == 2) {
			in_quota = 0;
			if (isspace(*ptr)) {
				in_space = 1;
				(*argvp)[arg_num] = strdup(tok);
				if (offsets) offsets[arg_num] = tok_offset;
				arg_num++;
				bzero(tok, MAX_TEXT_LEN);
				tok_pos = 0;
				tok_offset = 0;
			} else {
				in_space = 0;
			}
		} else if (in_space) {
			if (*ptr == '\"') {
				in_quota = 1;
			} else if (!isspace(*ptr)) {
				in_space = 0;
				if (tok_pos == 0) tok_offset = ptr - str;
				if (tok_pos < (MAX_TEXT_LEN-1))
					tok[tok_pos++] = *ptr;
			}
		} else {
			if (!isspace(*ptr)) {
				in_space = 0;
				if (tok_pos < (MAX_TEXT_LEN-1))
					tok[tok_pos++] = *ptr;
			} else {
				in_space = 1;
				(*argvp)[arg_num] = strdup(tok);
				if (offsets) offsets[arg_num] = tok_offset;
				arg_num++;
				bzero(tok, MAX_TEXT_LEN);
				tok_pos = 0;
				tok_offset = 0;
			}
		}
		++ptr;
		++len;
	}

	if (tok[0]) {
		(*argvp)[arg_num] = strdup(tok);
		if (offsets) offsets[arg_num] = tok_offset;
		arg_num++;
	}

	return arg_num;
}

/*
 * debug argv structure
 */
void
debug_argv(char **argv)
{
	int	i, n = 0;

	if (argv == NULL) return;

	for (i = 0; i <= MAX_ARG_NUM; i++) {
		if (argv[i]) {
			fprintf(stderr, "arg[%d] = %s\n", i, argv[i]);
			n++;
		} else
			break;
	}
	fprintf(stderr, "total %d args\n", n);
}

/*
 * free argv structure
 */
void
free_argv(char **argv)
{
	int	i;

	if (argv == NULL) return;

	for (i = 0; i <= MAX_ARG_NUM; i++) {
		if (argv[i]) {
			free(argv[i]);
		}
	}
	free(argv);
}

/*
 * display buf text by pages adapt to current screen width and height
 * This function is copyied from more.c of busybox prject
 */
int
display_buf_more(char *buf, int total)
{
	int c, lines, input = 0;
	int please_display_more_prompt = 0;
	int rows = 80, cols = 25;
	int i, len, page_height;
	int terminal_width;
	int terminal_height;
	struct termios init_termios;
	struct termios work_termios;

	if (!buf || !buf[0] || total <= 0) return -1;

	/* disable terminal echo */
	tcgetattr(0, &init_termios);
	memcpy(&work_termios, &init_termios, sizeof(struct termios));
	work_termios.c_lflag &= ~(ICANON | ECHO);
	work_termios.c_cc[VMIN] = 1;
	work_termios.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &work_termios);

	/* set proper screen width and height */
	rl_get_screen_size(&rows, &cols);
	please_display_more_prompt = 2;
	terminal_width = cols - 1;
	terminal_height = rows - 1;

	i = 0;
	len = 0;
	lines = 0;
	page_height = terminal_height;
	while (i < total && (c = buf[i++]) != '\0') {
		if ((please_display_more_prompt & 3) == 3) {
			len = printf("--More-- ");
			fflush(stdout);

			/* call customized getc with terminal timeout */
			input = ocli_rl_getc(stdin);

			/* Erase the "More" message */
			printf("\r%*s\r", len, "");
			len = 0;
			lines = 0;
			/* Bottom line on page will become top line
			 * after one page forward. Thus -1: */
			page_height = terminal_height - 1;
			please_display_more_prompt &= ~1;

			if (input != ' ' && input != '\n')
				break;
		}

		if (c == '\n') {
			/* increment by just one line if we are at
			 * the end of this line */
			if (input == '\n')
				please_display_more_prompt |= 1;
			/* Adjust the terminal height for any overlap, so that
			 * no lines get lost off the top. */
			if (len >= terminal_width) {
				int quot, rem;
				quot = len / terminal_width;
				rem  = len - (quot * terminal_width);
				page_height -= (quot - 1);
				if (rem)
					page_height--;
			}
			if (++lines >= page_height) {
				please_display_more_prompt |= 1;
			}
			len = 0;
		}

		putc(c, stdout);
		len++;
	}
	fflush(stdout);

	/* restore terminal echo */
	tcsetattr(0, TCSANOW, &init_termios);
	return 0;
}

/*
 * display file text by pages adapt to current screen width and height
 */
int
display_file_more(char *path)
{
	struct stat stat_buf;
	char	*addr = NULL;
	int	length = 0;
	int	res;
	int	fd;

	if (!path || !path[0]) return -1;

	bzero(&stat_buf, sizeof(stat_buf));
	if (stat(path, &stat_buf) != 0 || !S_ISREG(stat_buf.st_mode)) {
		fprintf(stderr, "display_file_more: bad file \'%s\'\n", path);
		return -1;
	}
	length = stat_buf.st_size;
	if (length < 1) return 0;

	if ((fd = open(path, O_RDONLY)) < 0) {
		fprintf(stderr, "display_file_more: open file \'%s\': %s\n",
			path, strerror(errno));
		return -1;
	}
	addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED) {
		fprintf(stderr, "display_file_more: mmap file \'%s\': %s\n",
			path, strerror(errno));
		close(fd);
		return -1;
	}

	res = display_buf_more(addr, length);
	munmap(addr, length);
	close(fd);
	return res;
}
