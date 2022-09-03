# 2. Libocli 符号定义和回调传参

中文 | [English](Symbol%20Definition.md)
<br>
[<< 上一级目录](README.zh_CN.md)  

作者：Digger Wu (digger.wu@linkbroad.com)

在[快速入门指南](Quick%20Start%20Guide.zh_CN.md)中，我们介绍了定制一个命令行的第一步就是要先定义符号表。符号表是包含多个 symbol_t 元素的数组，而 symbol_t 是个包含了多个元素的 struct，如果直接手写初始化/赋值会使得程序冗长，因此 Libocli 提供了几个宏，有助于减少初始化的代码量，并提升程序可读性。相关数据结构和宏定义可参考 [ocli.h](src/ocli.h) 。

## 2.1 定义一个符号

以下列出定义符号所用的宏定义。
| 宏名 | 说明 | 宏参 |
| --- | --- | --- |
| DEF_KEY | 定义一个关键字类型符号 | 语法符号，帮助文本 |
| DEF_KEY_ARG | 定义一个关键字类型符号，带回调 ARG | 语法符号，帮助文本，回调变量名 |
| DEF_VAR | 定义一个可变参数类型符号 | 语法符号，帮助文本，词法类型，回调变量名 |
| DEF_VAR_RANGE | 定义一个可变参数类型符号，带数值范围校验 | 语法符号，帮助文本，数值词法类型，回调变量名，最小值，最大值

宏参数规则以及范例：
- 所有宏的首两个参数都是：符号名，帮助文本，这个两个参数都是字符串。帮助文本用于在命令行交互中使用 '?' 列出本词或下一词的帮助信息。例如首个关键字符号 "ping" 的定义：
  > DEF_KEY ("ping",	"Ping utility")
- 回调变量名 对于 DEF_* 是必选的，对于 DEF_VAR* 则是可选的，多个符号可以使用相关的回调变量。
- 所有 DEF_VAR 必须要指定词法类型，比如 LEX_IP_ADDR，LEX_INT，等等。例如 HOST 是域名格式，但是 HOST_IP 是 IP 地址格式，两者都使用了相同的回调变量 DST_HOST：
  > DEF_VAR ("HOST_IP",	"Destination IP address", LEX_IP_ADDR,	ARG(DST_HOST)),  
  > DEF_VAR		("HOST",	"Destination domain name", LEX_DOMAIN_NAME, ARG(DST_HOST))
- DEF_RAR_RANGE 必须指定数值参数类型，LEX_INT 或 LEX_DECIMAL，最后给出最小和最大值限制，例如 COUNT 符号（ping 请求报文次数限制）的定义，限定 1-100 个报文：
- > DEF_VAR_RANGE	("COUNT",	"<1-100> count of requests", LEX_INT,	ARG(REQ_COUNT), 1, 100)

上述范例中定义回调参数时使用到 ARG 宏，ARG 宏的作用就是生成一个字符串，比如 ARG(DST_HOST)，就展开为 "DST_HOST"。

## 2.2 回调参数的传递

当命令行解析成功后，一个 cmd_arg_t 类型的数组将会被传递给回调函数，cmd_arg_t 包含两个元素：变量名name，和变量值 value。
```
typedef struct cmd_arg {
	char	*name;		/* arg name */
	char	*value;		/* arg value */
} cmd_arg_t;
```
比如执行：
>ping -c 5 www.bing.com

Libocli 的解析过程如下：
- "ping" 匹配关键字符号 "ping"
- “-c" 匹配关键字符号 "-c"
- "5" 匹配 "COUNT" 符号，且满足 1 < 5 < 100，因 COUNT 有回调参数 "REQ_COUNT"，那么生成一个回调参数元素 cmd_arg[0]，name = "REQ_COUNT", value = "5"
- “www.bing.com” 匹配了 "HOST" 符号，"HOST" 符号有回调参数 "DST_HOST"，那么生成一个回调参数元素 cmd_arg[1]，name = "DST_HOST", value = "www.bing.com"

解析完毕，cmd_arg[] 数组会被传递给回调函数 cmd_ping()，cmd_ping() 调用的 for_each_cmd_arg(cmd_arg, i, name, value)，实际上就是在遍历 cmd_arg[] 数组各元素，若 name 匹配元素，就取出其 value。

```
	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, REQ_COUNT))
			req_count = atoi(value);
		else if (IS_ARG(name, DST_HOST))
			strncpy(dst_host, value, sizeof(dst_host)-1);
		/* ... */
	}
```

上述代码段中的 IS_ARG 宏的作用是做一个名字匹配检查，比如 IS_ARG(name, REQ_COUNT) 会展开为 strcmp(name, "REQ_COUNT") == 0 
