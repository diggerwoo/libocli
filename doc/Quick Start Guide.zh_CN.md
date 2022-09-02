# 1. Libcli 快速入门

中文 | [English](Quick%20Start%20Guide.md)
<br>
[<< 上一级目录]("API%20Manual.zh_EN.md")  

## 1.1 概览
Libocli 本身并不实现终端字符读取、编辑功能，此方面功能直接利用 GNU Readline。
GNU Readline 是个非常全面的终端行编辑库 / 框架，提供了所有的命令行编辑能力，包括且不限于：Emacs 风格编辑快捷键、关键字 TAB 补齐、双 TAB 列出下一关键字序列、命令行历史记录、等等。
Libocli 其实就是封装了 GNU Readline 实现的一套带有词法、语法解析的外挂。使用 Libocli 开发命令行程序时，你只要专注于：注册一个命令行语法，实现该命令行对应回调业务函数。

## 1.2 注册一个命令以及语法
例如设计一个简单的 ping 命令语法，可以指定三个选项：
- ping 之后的两个选项：-c 指定发送 ICMP Echo 报文次数，-s 指定报文长度
- 最后可选的 from 子句，可以指定 ping 的 IP 源地址  

按 Linux man 的惯常写法，语法表达如下：
>ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]  

以下演示注册一个 ping 语法，程序片段摘自 [example/netutil.c](../example/netutil.c)  

1. 制作一个符号表：
```
/* 定义 "ping" 命令的符号表 syms_ping，包括关键字，和变量 */
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

2. 创建命令，注册语法：
```
int cmd_net_utils_init()
{
	struct cmd_tree *cmd_tree;
        
	/* 创建 "ping" 命令，指定符号表 syms_ping，回调函数 cmd_ping() */
	cmd_tree = create_cmd_tree("ping", SYM_TABLE(syms_ping), cmd_ping);
        
	/* 注册一条语法，并且按此语法创建对应的帮助文本，可以 "man ping" 查看 */
	add_cmd_easily(cmd_tree, "ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]",
		       (ALL_VIEW_MASK & ~BASIC_VIEW), DO_FLAG);
        return 0;
}
```

3. 实现回调业务函数：

TODO

## 1.3 主程序流程
以下主程序片段摘自 [example/democli.c](../example/democli.c)，演示了如何利用 Libocli 初始化、启动命令行解析：


```
/* libocli 头文件 */
#include <ocli/ocli.h>

int main()
{
	/* 初始化 libocli */
	ocli_rl_init();

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

	/* 终端 5 分钟空闲，自动退出，这是专业命令行必备的安全设置项 */
	ocli_rl_set_timeout(300);
    
	/* 终端空闲超时，或者 CTRL-D，自动执行 "exit" 命令 */
	ocli_rl_set_eof_cmd("exit");

	/* 设置初始权限视图为 BASIC_VIEW  */
	ocli_rl_set_view(BASIC_VIEW);
	set_democli_prompt(BASIC_VIEW);

	/* 开始 libocli 命令行读取循环 */
	ocli_rl_loop();

	/* 退出 libocli，恢复程序启动前的的终端设置 */
	ocli_rl_exit();
	return 0;
}
```
