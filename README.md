# libocli
libocli is a C library to provide Cisco style command line interface. It is developed on Linux and depends on GNU readline and pcre libs.

Main features of libocli includes:
- Line editing keys, TAB for auto keyword completion, and '?’ for next help.
- Commonly used lexical types including: IP, IPv6, MAC address, Domain Name, URLs, etc.
- Easy-to-use command syntax building APIs.
- Support multi-view, and builtin "no" command.
- Builtin "man" command to show all syntaxes of a command. Cisco IOS lacks this while we need it.

Below shows a example to briefly describe how to build a ping command with libocli.
The ping command syntax is designed as:
> ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]

```
#include "ocli.h"

/* declare all keyword and variable symbols for ping command */
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
        DEF_END
};

static int cmd_ping(cmd_arg_t *cmd_arg, int do_flag);

int
cmd_ping_init()
{
	struct cmd_tree *cmd_tree;
        
        /* initilize the symbol table */
	if (prepare_symbols(&symbols[0]) < 0) {
		cleanup_symbols(&symbols[0]);
		return -1;
	}
        
	/* create a syntax tree for "ping", callback to cmd_ping() */
	cmd_tree = create_cmd_tree("ping", &symbols[0], cmd_ping);

	/* add a syntax, also create the manual which can be displayed by "man ping" */
	add_cmd_easily(cmd_tree, "ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]",
		       ALL_VIEW_MASK, DO_FLAG);
	retuturn 0;
}

/* callback function of ping */
static int
cmd_ping(cmd_arg_t *cmd_arg, int do_flag);
{
	int	i;
	char	*name, *value;
	int	req_count = 5;	/* default 5 echo requests */
	int	pkt_size = 56;	/* default 56 bytes packet */
	char	dst_host[128];
	char	local_addr[128];
	char	cmd_str[256];

	bzero(dst_host, sizeof(dst_host));
	bzero(local_addr, sizeof(local_addr));

	/* arg parsing */
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

	/* compose the actual ping command and exec it */
	snprintf(cmd_str, sizeof(cmd_str), "/bin/ping -c %d -s %d %s %s %s",
		req_count, pkt_size,
		local_addr[0] ? "-I":"",
		local_addr[0] ? local_addr:"",
		dst_host); 
	return system(cmd_str);
}
```






