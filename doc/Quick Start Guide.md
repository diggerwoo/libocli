# 1. Libocli Quick Start Guide

English | [中文](Quick%20Start%20Guide.zh_CN.md)
<br>
[<< Upper Level](README.md)  

Author: Digger Wu (digger.wu@linkbroad.com)

Libocli itself does not implement the terminal line editing functions, it depends on GNU readline which has rich features including: Emacs keys, TAB  for auto completion, double TABs for next help list, ARROW UP/DOWN for history, etc.

Libocli is actually an add-on which encapsulates GNU Readline to provide command lexcial parsing, syntax parsing and callback excecution.
By utilizing Libocli, develpers need only to focus on command syntax design, and callback implementation.
The example code in this section shows how to use Libocli to quickly build a ping command.

## 1.1 Create a command and register syntaxes

The following code segments are taken from [example/netutils.c](../example/netutils.c) which implements a ping CLI with simple options like Linux ping. The options and paramter are defined as below:
- Two options："-c" to specify the number of ICMP Echo requests, and "-s" to specify the length of ICMP packet.
- Madatory destination host parameter: either IP address or domain name format.
- Optional "from" clause: to specify the source interface IP address.

We can represent the syntax as below in accordance with the convention of Linix manual:
>ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]  

### 1.1.1 Define a symbol table

Before the creation of ping command, we shoud define all the symbols included in the above syntax first, include keywords: "ping" "-c" "-s" "from", and variable paramaters ”COUNT“ ”SIZE" "HOST" "HOST_IP", with exception of syntax anchors "[ ] { | }".
```
/*
 * Symbol table:  syms_ping
 * DEF_KEY Macro: Defines a symbol of Keyword type
 * DEF_VAR Macro: Defines a symbol of Variable type
 * ARG Macro:     Defines a callback argument, which will be delivered to callback function if parsing successful
 * LEX_* Macros:  Define the lexcical type, e.g LEX_IP_ADDR means IP address format.
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

Then based on the symbol table, we create the ping command, and register a syntax. Please note that all words in the syntax string should be previously defined in the symbol table, or else error occurs.
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

Now we can implement the callback function. The callback function parses and gets all the ARGs defined previously in the symbol table, then compose an external ping command, finally excecutes it.

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

Below code segment is taken from [example/democli.c](../example/democli.c). The main() function is quite straitforward by utlizing the libocli APIs: call ocli_rl_init() to initialize libocli runtime environment，then call bunch of cmd_xxx_init() functions to create commands and syntaxes，finally call ocli_rl_loop() to parse commands and execute callbacks until exit.

```
/* libocli header */
#include <ocli/ocli.h>

int main()
{
	/* Always init ocli_rl_init first */
	ocli_rl_init();

	/* For the sake of security, exit if terminal idled for 5 minutes */
	ocli_rl_set_timeout(300);

	/* Create libocli builtin commands: "man" and "no" */
	cmd_manual_init();
	cmd_undo_init();

	/* Create customized "enable", "configure", and "exit" commands */
	cmd_sys_init();
	/* Create customized "ping" and "trace-route" commands */
	cmd_net_utils_init();
	/* Create customized "route" command */
	cmd_route_init();
	/* Create customized "show" commands */
	cmd_show_init();

	/* Auto exec "exit" for when CTRL-D being pressed */
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
