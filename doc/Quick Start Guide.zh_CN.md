# 1. Libocli 快速入门

中文 | [English](Quick%20Start%20Guide.md)
<br>
[<< 上一级目录](README.zh_CN.md)  

作者：Digger Wu (digger.wu@linkbroad.com)

Libocli 本身并不实现终端行编辑功能，而是直接利用 GNU Readline。
GNU Readline 是个非常强的终端行编辑库和框架，提供了丰富的命令行编辑能力，包括且不限于：Emacs 风格编辑快捷键、关键字 TAB 补齐、双 TAB 列出下一关键字序列、命令行历史记录、等等。
Libocli 其实就是在 GNU Readline 之上实现的可定制词法、语法解析和命令回调的一套外挂。
使用 Libocli 开发命令行程序时，开发者只要专注于：注册一个命令行和以及语法，实现该命令行对应的回调业务函数。
以下用一个 ping 的例子简述如何使用 Libocli 快速构建一个带有可选项语法的命令行。

## 1.1 注册一个命令行以及语法

本例程序片段摘自 [example/netutils.c](../example/netutils.c)  

例子中设计了一个简单的 ping 命令语法，可指定三个选项：
- ping 关键字之后两个可选项："-c" 指定发送 ICMP Echo 报文次数，"-s" 指定报文长度
- ping 目的地址参数后，可选的 "from" 子句，指定 ping 的 IP 源地址  

按 Linux 手册的惯常写法，上述 ping 语法可表示为：
>ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]  

### 1.1.1 定义一个符号表

实现一个命令之前，我们需要先定义符号表，符号表中要包含 ping 命令语法所需的所有符号。

```
/*
 * 定义符号表： syms_ping
 * DEF_KEY 宏：定义一个关键字类型符号
 * DEF_VAR 宏：定义一个变量类型符号，DEF_VAR_RANGE：定义一个带数值范围的变量类型符号
 * ARG 宏：    定义该符号对应的回调参数，若命令最终解析成功，该参数会被传递给回调函数
 * LEX_* 宏：  定义变量类型符号的词法类型，比如 LEX_IP_ADDR 表示 IP 地址格式
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

### 1.1.2 创建命令，并注册语法

之后，我们基于符号表创建命令、注册语法。注册语法串中出现的所有单词都应该是符号表中预先定义过的，否则会注册失败。
```
int cmd_net_utils_init()
{
	struct cmd_tree *cmd_tree;
        
	/* 创建 "ping" 命令，符号表为 syms_ping，回调函数为 cmd_ping() */
	cmd_tree = create_cmd_tree("ping", SYM_TABLE(syms_ping), cmd_ping);
        
	/* 注册一条语法，并且按此语法创建对应的帮助文本，可供 "man ping" 查看 */
	add_cmd_easily(cmd_tree, "ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]",
		       (ALL_VIEW_MASK & ~BASIC_VIEW), DO_FLAG);
        return 0;
}
```

### 1.1.3 实现回调业务函数

现在我们可以实现回调函数了。回调函数尝试解析所有的回调参数（符号表里 ARG 宏定义的），然后拼装出一条系统 ping 命令，并执行。
```
static int cmd_ping(cmd_arg_t *cmd_arg, int do_flag)
{
	int	i;
	char	*name, *value;
	int	req_count = 5;	/* 默认 5 个 ICMP Echo 请求报文 */
	int	pkt_size = 56;	/* 默认 56 字节报文长度 */
	char	dst_host[128];
	char	local_addr[128];
	char	cmd_str[256];

	bzero(dst_host, sizeof(dst_host));
	bzero(local_addr, sizeof(local_addr));

	/* 使用 for_each_cmd_arg() 和 IS_ARG() 宏，解析符号表中所定义的变量 */
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

	/* 组装出一条 Linux 的 ping 命令，并执行 */
	snprintf(cmd_str, sizeof(cmd_str), "ping -c %d -s %d %s %s %s",
		req_count, pkt_size,
		local_addr[0] ? "-I":"",
		local_addr[0] ? local_addr:"",
		dst_host); 
	return system(cmd_str);
}
```

## 1.2 主程序流程

以下主程序片段摘自 [example/democli.c](../example/democli.c)。主程序的写法很简单，ocli_rl_init() 初始化，然后调用各个模块的 cmd_xxx_init() 注册命令和语法，之后 ocli_rl_loop() 启动命令行读取循环，执行所有的命令解析和回调，直到程序退出：

```
/* libocli 头文件 */
#include <ocli/ocli.h>

int main()
{
	/* 初始化 libocli */
	ocli_rl_init();

	/* 终端 5 分钟空闲，自动退出，这通常是专业命令行必备的安全设置项 */
	ocli_rl_set_timeout(300);
    
	/* 启用 libocli 内置命令： "man" 和 "no" */
	cmd_manual_init();
	cmd_undo_init();

	/* 创建自定义的系统命令： "enable", "configure", and "exit" */
	cmd_sys_init();
	/* 创建自定义命令 "ping" 和 "trace-route"  */
	cmd_net_utils_init();
	/* 创建自定义命令 "route" */
	cmd_route_init();
	/* 创建自定义命令 "show" */
	cmd_show_init();

	/* 若按下 CTRL-D，自动执行 "exit" 命令，就像 bash 那样  */
	ocli_rl_set_eof_cmd("exit");

	/* 设置初始权限视图为 BASIC_VIEW  */
	ocli_rl_set_view(BASIC_VIEW);
	set_democli_prompt(BASIC_VIEW);

	/* 开始 libocli 命令行读取主循环 */
	ocli_rl_loop();

	/* 退出 libocli，恢复程序启动前的的终端设置 */
	ocli_rl_exit();
	return 0;
}
```
