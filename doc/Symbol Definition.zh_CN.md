# 2. Libocli 符号定义和回调传参

中文 | [English](Symbol%20Definition.md)
<br>
[<< 上一级目录](README.zh_CN.md)  

作者：Digger Wu (digger.wu@linkbroad.com)

在[快速入门指南](Quick%20Start%20Guide.zh_CN.md)中，我们介绍了开发一个命令行的第一步就是要先定义符号表。符号表是包含多个 symbol_t 元素的数组，而 symbol_t 是个包含多个元素的 struct，如果直接手写初始化 / 赋值会使得程序冗长，因此 Libocli 随附了一些宏工具，用于减少代码量，并提升程序可读性。相关数据结构和宏定义可参考 [ocli.h](../src/ocli.h) 。

## 2.1 定义一个符号

以下列出定义符号所用的 DEF_ 宏。
| 宏名称 | 作用 | 宏参数 |
| :--- | :--- | :--- |
| DEF_KEY | 定义关键字类型符号 | (符号，帮助文本) |
| DEF_KEY_ARG | 定义关键字类型符号，带回调参数 | (符号，帮助文本，回调变量名) |
| DEF_VAR | 定义变量类型符号 | (符号，帮助文本，词法类型，回调变量名) |
| DEF_VAR_RANGE | 定义数值变量类型符号，带数值范围校验 | (符号，帮助文本，数值词法类型，回调变量名，最小值，最大值) |

DEF_ 宏参数规则以及范例：
- 所有宏的首两个参数都是：符号名，帮助文本，这个两个参数都是字符串。帮助文本用于在命令行交互中使用 '?' 列出本词或下一词的帮助信息。例如首个关键字符号 "ping" 的定义：
  > 
  ```c
  DEF_KEY ("ping", "Ping utility")
  ```
- DEF_VAR 必须要指定词法类型，比如 LEX_IP_ADDR，LEX_DOMAIN_NAME，LEX_INT，等等。详细的词法类型可参考下一节 [Libocli 词法分析接口](Lexical%20Parsing.zh_CN.md)。

- 除了 DEF_KEY，其它 DEF_ 宏都必须指定回调变量名，回调变量名是个字符串，例子程序中我们使用 ARG 宏来定义回调变量名，ARG 宏的作用就是将参数展开为一个字符串，下例中的  ARG(DST_HOST)，会展开为 "DST_HOST"
  ```c
  /* 注意：因为 ping 语法的 HOST_IP 和 HOST 符号二者只能匹配其一，
   * 所以两符号可使用相同的回调参数名 "DST_HOST"，简化回调函数的参数处理
   */
  DEF_VAR ("HOST_IP", "Destination IP address", LEX_IP_ADDR, ARG(DST_HOST)),  
  DEF_VAR ("HOST", "Destination domain name", LEX_DOMAIN_NAME, ARG(DST_HOST))
  ```
- DEF_VAR_RANGE 必须指定数值词法类型（LEX_INT 或 LEX_DECIMAL），并给出最小和最大数值参数，下例中 COUNT 符号（ping 请求报文次数限制）限定输入 1-100 整型：
  >
  ```c
  DEF_VAR_RANGE	("COUNT", "<1-100> count of requests", LEX_INT, ARG(REQ_COUNT), 1, 100)
  ```

## 2.2 回调参数的传递

当命令行解析成功后，一个 cmd_arg_t 类型的数组将会被传递给回调函数，cmd_arg_t 包含两个元素：变量名 name，和变量值 value。
```c
typedef struct cmd_arg {
	char	*name;		/* arg name */
	char	*value;		/* arg value */
} cmd_arg_t;
```
比如执行如下 ping 命令行：
>ping -c 5 www.bing.com

Libocli 的解析过程如下：
1. "ping" 匹配关键字符号 "ping"
2. “-c" 匹配关键字符号 "-c"
3. "5" 匹配 "COUNT" 符号，且满足 1 < 5 < 100，因 COUNT 带有回调参数 "REQ_COUNT"，那么生成第一个回调参数元素 cmd_arg[0]，name 指向 "REQ_COUNT", value 指向 "5"
4. “www.bing.com” 匹配 "HOST" 符号，"HOST" 符号带有回调参数 "DST_HOST"，那么生成第二个回调参数元素 cmd_arg[1]，name 指向 "DST_HOST", value 指向 "www.bing.com"

解析完毕，cmd_arg[] 数组会被传递给回调函数 cmd_ping()，cmd_ping() 的 for_each_cmd_arg(cmd_arg, i, name, value) 循环就是在遍历 cmd_arg[] 数组，若数组元素 name 匹配，则取出其 value。

```c
	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, REQ_COUNT))
			req_count = atoi(value);
		else if (IS_ARG(name, DST_HOST))
			strncpy(dst_host, value, sizeof(dst_host)-1);
		/* ... */
	}
```

上例代码段中的 IS_ARG 宏与符号定义中的 ARG 宏配合，目的仍然是简化程序和增加可读性。IS_ARG 宏的作用是就是调用 strcmp() 做一个名字匹配检查，比如 IS_ARG(name, REQ_COUNT) 会展开为 (strcmp(name, "REQ_COUNT") == 0)，匹配则返回真。
