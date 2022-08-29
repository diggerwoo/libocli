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

/* Define all keyword / variable symbols for "ping" and "trace-route" */
static symbol_t symbols[] = {
	DEF_KEY         ("ping",	"Ping utility"),
	DEF_KEY		("-c",		"Set count of requests"),
	DEF_VAR_RANGE	("COUNT",	"<1-100> count of requests",
			 LEX_INT,	ARG(REQ_COUNT), 1, 100),
	DEF_KEY		("-s",		"Set size of packet"),
	DEF_VAR_RANGE	("SIZE",	"<22-2000> size of packet",
			 LEX_INT,	ARG(PKT_SIZE), 22, 2000),
	DEF_VAR		("HOST",	"Destination domain name",
			 LEX_DOMAIN_NAME, ARG(DST_HOST)),
	DEF_VAR		("HOST_IP",	"Destination IP address",
			 LEX_IP_ADDR,	ARG(DST_HOST)),
	DEF_KEY		("from",	"Set ping source address"),
	DEF_VAR		("IFADDR",	"Interface IP address",
			 LEX_IP_ADDR,	ARG(LOCAL_ADDR)),
	/* trace route reuses above HOST* args of ping */
	DEF_KEY         ("trace-route",	"Trace route utility"),
	DEF_END
};

static int cmd_ping(cmd_arg_t *cmd_arg, int do_flag);
static int cmd_trace(cmd_arg_t *cmd_arg, int do_flag);

int
cmd_net_utils_init()
{
	struct cmd_tree *cmd_tree;
        
        /* Initilize the symbol table */
	if (prepare_symbols(&symbols[0]) < 0) {
		cleanup_symbols(&symbols[0]);
		return -1;
	}
        
	/* Create a syntax tree for "ping", callback to cmd_ping() */
	cmd_tree = create_cmd_tree("ping", &symbols[0], cmd_ping);
        
	/* Add a syntax, also create the manual which can be displayed by "man ping" */
	add_cmd_easily(cmd_tree, "ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]",
		       ALL_VIEW_MASK, DO_FLAG);

	/*
	 * Create a syntax tree for "trace-route", callback to cmd_trace()
	 * For demo purpose, "trace-route" is only accessible in ENABLE_VIEW
	 */
	cmd_tree = create_cmd_tree("trace-route", &symbols[0], cmd_trace);
	add_cmd_easily(cmd_tree, "trace-route { HOST | HOST_IP }",
		       ENABLE_VIEW, DO_FLAG);
	return 0;
}

/* Callback function of "ping" command */
static int
cmd_ping(cmd_arg_t *cmd_arg, int do_flag)
{
	int	i;
	char	*name, *value;
	int	req_count = 5;	/* Default 5 ICMP echo requests */
	int	pkt_size = 56;	/* Default 56 bytes length packet */
	char	dst_host[128];
	char	local_addr[128];
	char	cmd_str[256];

	bzero(dst_host, sizeof(dst_host));
	bzero(local_addr, sizeof(local_addr));

	/* Parse args by for_each_cmd_arg() and IS_ARG() macros */
	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, REQ_COUNT))
			req_count = atoi(value);
		else if (IS_ARG(name, PKT_SIZE))
			pkt_size = atoi(value);
		else if (IS_ARG(name, DST_HOST))
			strncpy(dst_host, value, sizeof(dst_host)-1);
		else if (IS_ARG(name, LOCAL_ADDR))
			strncpy(local_addr, value, sizeof(local_addr)-1);
	}

	/* Compose the actual ping command then exec it */
	snprintf(cmd_str, sizeof(cmd_str), "ping -c %d -s %d %s %s %s",
		req_count, pkt_size,
		local_addr[0] ? "-I":"",
		local_addr[0] ? local_addr:"",
		dst_host); 
	return system(cmd_str);
}

/* Callback function of "trace-route" command */
static int
cmd_trace(cmd_arg_t *cmd_arg, int do_flag)
{
	int	i;
	char	*name, *value;
	char	dst_host[128];
	char	cmd_str[256];

	bzero(dst_host, sizeof(dst_host));

	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, DST_HOST))
			strncpy(dst_host, value, sizeof(dst_host)-1);
	}

	snprintf(cmd_str, sizeof(cmd_str), "traceroute -n %s", dst_host);
	return system(cmd_str);
}
