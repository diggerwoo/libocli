# 1. Libocli 快速入门指南

中文 | [English](Quick%20Start%20Guide.md)
<br>
[<< 上一级目录](README.zh_CN.md)  

作者：Digger Wu (digger.wu@linkbroad.com)

Libocli 直接调用 GNU Readline 的终端行编辑功能。
GNU Readline 的行编辑功能非常丰富，包括且不限于：Emacs 风格编辑快捷键、关键字 TAB 补齐、双 TAB 列出下一关键字序列、命令行历史记录、等等。
Libocli 其实就是在 GNU Readline 之上构建的，可实现词法分析、语法分析、命令回调的一套外挂。
使用 Libocli 开发命令行程序时，开发者只要专注于：注册一个命令行和以及语法，实现该命令行对应的回调业务函数。
以下用一个 ping 的例子简述如何使用 Libocli 快速构建一个带有可选项语法的命令行。

## 1.1 快速实现一个 ping 命令行

本例程序片段摘自 [example/netutils.c](../example/netutils.c)，例子中设计了一个简单的 Linux 选项风格的 ping 命令语法，选项和参数依次定义如下： 
- 两个含参可选项："-c" 指定发送 ICMP Echo 报文次数，"-s" 指定报文长度
- 目的地址参数，格式可以是 IP 地址，或者域名
- 可选的 "from" 子句，可指定 ping 的接口源 IP 地址  

按 Linux 手册的惯常写法，上述 ping 语法可表达为：
>
```sh
ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]
```

### 1.1.1 定义符号表

实现上述 ping 命令之前，我们首先要定义符号表，表中包含 ping 语法串中的所有单词，包括关键字类型符号："ping" "-c" "-s" "from"，变量类型符号：”COUNT“ ”SIZE" "HOST" "HOST_IP" "IFADDR"。注意不要使用 Libocli 的保留语法标记： "[ ] { | }" 。

```c
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

### 1.1.2 创建命令并注册语法

之后，我们在 cmd_net_utils() 函数中，基于符号表创建命令、注册语法。注册语法串中出现的所有单词都应该是符号表中预先定义好的，否则会注册失败。
```c
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

现在我们可以实现回调函数 cmd_ping() 了。回调函数尝试解析所有的回调参数（符号表里 ARG 宏定义的），然后组装出一条 Linux 系统 ping 命令，并执行。
```c
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

	/* 使用 for_each_cmd_arg() 和 IS_ARG() 宏，解析符号表中所定义的回调参数 */
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

## 1.2 与主程序整合

通常我们开发命令行时，会按特定应用目的将各命令分类实现在不同的业务模块。在 democli 例子程序里，我们指定 netutils.c 去实现一些网络工具，比如 "ping" 和 "traceroute"，或许我们还会在这个模块里加入其它工具，比如 "nslookup"、"ssh" 等等。之后我们在这个业务模块里，实现一个初始化函数 cmd_xxx_init()，注册本模块所有的命令和语法，如上 cmd_net_utils_init() 所示。最后，我们将这个新增的 cmd_xxx_init() 整合到主程序流程，重新编译程序，测试。

以下主程序片段摘自 [example/democli.c](../example/democli.c)。使用 Libocli 后，主程序的写法就很简单了：
- ocli_rl_init() 初始化
- 注册所有的命令和语法，在 democli 这个例子里，命令注册流程都实现在各个模块的 cmd_xxx_init() 函数里，且这些函数都已在主程序头文件 [democli.h](../example/democli.h) 中声明，包括了注册 ping 命令语法的 cmd_net_utils_init()
- 设置必要的命令行控制选项，包括命令行超时时间、初始权限视图、提示符，等等
- 调用 ocli_rl_loop() 启动命令行读取循环、执行所有的命令解析和回调，直至程序退出

```c
/* libocli 头文件 */
#include <ocli/ocli.h>
#include "democli.h"

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

	/* 若按下 CTRL-D，自动执行 "exit" 命令，就像 bash 那样  */
	ocli_rl_set_eof_cmd("exit");

	/* 终端 5 分钟空闲，自动退出，这通常是专业命令行必备的安全设置项 */
	ocli_rl_set_timeout(300);
    
	/* 设置初始权限视图为 BASIC_VIEW  */
	democli_set_view(BASIC_VIEW);

	/* 开始 libocli 命令行读取主循环 */
	ocli_rl_loop();

	/* 退出 libocli，恢复程序启动前的的终端设置 */
	ocli_rl_exit();
	return 0;
}
```
## 1.3 测试 ping 命令


整合完后，我们执行 democli 测试看看 ping 命令的执行效果.

![image](https://github.com/diggerwoo/blobs/blob/main/img/democli_ping.gif)
