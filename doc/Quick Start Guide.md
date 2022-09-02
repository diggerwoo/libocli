# 1. Libcli Quick Start Guide

English | [中文](Quick%20Start%20Guide.zh_CN.md)
<br>
[<< Upper Level](README.md)  

Author: Digger Wu (digger.wu@linkbroad.com)

Libocli itself does not implement VTY I/O or editing fucntions, it depends on GNU readline which has strong capacity including: EMACS keys, TAB auto completion, double TABs help list, line history, etc.

Libocli is actuall an add-on which encapsulate GNU Readline to provide command lexcial parsing, syntax parsing and callback excecution.
By utilizing Libocli, develper need only focus on CLI syntax design, and callback implementaion.
The example codes in this section shows how to use Libocli to quickly build a ping command.

## 1.1 Create a command and register syntaxes

Below codes are from [example/netutils.c](../example/netutils.c)  

It gives a example ping CLI, which has thres options:
- Options after "ping" keyword：-c to specify number of ICMP echo requests, -s to specify packet length.
- "from" clause after destination host, to specify ping source interface address.

Like the convention of Linix manual, the syntax can be represents as:
>ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]  

### 1.1.1 Define a symbol table
```
/*
 * symbol table: syms_ping
 * DEF_KEY Macro: Define a symbol of Keyword type
 * DEF_VAR Macro: Define a symbol of Variable type
 * ARG Macro: Define a callback arg, which will be delivered to callback function if command parsing successful
 * LEX_* Macro: Define the lexcical type, for details reference src/lex.h
 */
static symbol_t syms_ping[] = {
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
			 LEX_IP_ADDR,	ARG(LOCAL_ADDR))
};
```

### 1.1.2 Create command, and register syntaxes
```
int cmd_net_utils_init()
{
	struct cmd_tree *cmd_tree;
        
	/* Create "ping" command，with symbol table: syms_ping, and callback function: cmd_ping() */
	cmd_tree = create_cmd_tree("ping", SYM_TABLE(syms_ping), cmd_ping);
        
	/* Register the syntax，and auto generate manual for "man ping" */
	add_cmd_easily(cmd_tree, "ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]",
		       (ALL_VIEW_MASK & ~BASIC_VIEW), DO_FLAG);
        return 0;
}
```

### 1.1.3 Implement callback function

```
static int cmd_ping(cmd_arg_t *cmd_arg, int do_flag)
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

	/* Parse callback args by for_each_cmd_arg() and IS_ARG() macros */
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
```

## 1.2 Main program

Below codes are from [example/democli.c](../example/democli.c), which demonstrate how to initialize Libocli and start command parsing.

```
/* libocli header */
#include <ocli/ocli.h>

int main()
{
	/* Always init ocli_rl_init first */
	ocli_rl_init();

	/* For the sake of security, exit if terminal idled for 5 minutes */
	ocli_rl_set_timeout(300);

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

	/* Auto exec "exit" for when CTRL-D pressed */
	ocli_rl_set_eof_cmd("exit");

	/* Start from BASIC_VIEW */
	ocli_rl_set_view(BASIC_VIEW);
	set_democli_prompt(BASIC_VIEW);

	/* Loop to parsing and exec commands */
	ocli_rl_loop();

	/* Call ocli_rl_exit to restore original terminal attributes */
	ocli_rl_exit();
	return 0;
}
```
