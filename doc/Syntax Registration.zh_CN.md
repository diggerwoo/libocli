# 4. Libocli 命令和语法注册接口

中文 | [English](Syntax%20Register.md)
<br>
[<< 上一级目录](README.zh_CN.md)  

作者：Digger Wu (digger.wu@linkbroad.com)

## 4.1 创建命令

创建命令接口函数为 create_cmd_tree()，定义如下：

```c
#define SYM_NUM(syms) (sizeof(syms)/sizeof(symbol_t))
#define SYM_TABLE(syms) &syms[0], SYM_NUM(syms)

/* 回调函数类型 */
typedef int (*cmd_fun_t)(cmd_arg_t *, int);

/*
 * 返回值类型为 struct cmd_tree 类型指针，失败返回 NULL
 */
struct cmd_tree *
create_cmd_tree(char *cmd,              /* 命令关键字 */
                symbol_t *sym_table,    /* 符号表指针 */
                int sym_num,            /* 符号个数 */
                cmd_fun_t fun           /* 回调函数指针 */
                );
```

SYM_TABLE 宏可以简化 create_cmd_tree() 调用写法，如 [netutils.c](../example/netutils.c) 所示：
```c
static symbol_t syms_ping[] = {
        DEF_KEY ("ping",        "Ping utility"),
        DEF_KEY	("-c",          "Set count of requests"),
        /* ... */
};

static int cmd_ping(cmd_arg_t *cmd_arg, int do_flag);

int cmd_ping_init()
{
        struct cmd_tree *cmd_tree;
        cmd_tree = create_cmd_tree("ping", SYM_TABLE(syms_ping), cmd_ping);
        /* ... */
}
```

回调函数类型 cmd_fun_t 的参数：
- 第一个参数是 cmt_art_t 类型数组指针，指向命令行解析过程所产生的回调参数数组，在 [回调传参](Symbol%20Definition.zh_CN.md) 一节中有具体描述。  
- 第二个参数是个整型，例子中的回调函数都使用 do_flag 来命名这个参数。这个参数用于告知回调函数当下这个命令是常规的、不带 "no" 执行的（即 do_flag & DO_FLAG 为真），还是以 "no ..." 语法执行的（即 do_flag & UNDO_FLAG 为真）。很明显 ping 命令不需要 no 语法，但是配置命令可能需要，比如例子中的 route 命令，删除路由时需要 no route ...，参考 [route.c](../example/route.c)。

## 4.2 注册语法

注册语法接口函数为 add_cmd_easily() 和 add_cmd_syntax()，定义如下：
```c
/* 预定义的视图，每个视图独立占一位 */
#define	BASIC_VIEW		0x0001
#define	ENABLE_VIEW		0x0002
#define	CONFIG_VIEW		0x0004

/* do_flag 参数的的位定义，仅当需要 "no" 语法，置位 UNDO_FLAG */
#define	DO_FLAG		0x01
#define	UNDO_FLAG	0x02

/* 注册语法，并自动生成手册文本，成功返回 0，否则返回 -1 */
int add_cmd_easily(struct cmd_tree *cmd_tree,   /* create_cmd_tree返回的语法树指针 */
                   char *syntax,                /* 语法字符串 */
                   int view_mask,               /* 本语法的视图掩码 */
                   int do_flag                  /* DO_FLAG 和 UNDO_FLAG 的位或组合 */
                   );

/* 仅注册语法，参数和返回定义同上 add_cmd_easily() */
int add_cmd_syntax(struct cmd_tree *cmd_tree,
                   char *syntax,
                   int view_mask,
                   int do_flag);

```

以下给出几个典型应用范例：

1. 注册一条 ping 语法如下，见 [netutils.c](../example/netutils.c)，这条命令语法只能 enable 后才能访问（除 BASIC_VIEW 之外的所有视图），不支持 "no" 语法（do_flag 赋值 DO_FLAG）。
    ```c
    add_cmd_easily(cmd_tree, "ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]",
                   (ALL_VIEW_MASK & ~BASIC_VIEW), DO_FLAG);
    ```

2. 分别在不同视图中注册 "enable" 和 "enable password" 语法，见 [democli.c](../example/democli.c)。
    ```c
    /* 注册一条 "enable" 语法，只能在 BASIC_VIEW 视图访问，用于输入 enable 密码后提权至 ENABLE_VIEW */
    add_cmd_easily(cmd_tree, "enable", BASIC_VIEW, DO_FLAG);

    /* 注册一条 "enable password" 语法，只能在 ENABLE_VIEW 中访问，用于修改 enable 密码 */
    add_cmd_easily(cmd_tree, "enable password", ENABLE_VIEW, DO_FLAG);
    ```

3. 注册支持 "no" 的 "route" 语法，注意 do_flag 赋值 DO_FLAG|UNDO_FLAG，此条语法只能在 CONFIG_VIEW 视图中访问，见 [route.c](../example/route.c)。
    ```c
    add_cmd_easily(cmd_tree, "route DST_NET DST_MASK GW_ADDR", CONFIG_VIEW, DO_FLAG|UNDO_FLAG);
    ```

4. 上例中，你可能觉得 no route 删除路由时，最后一个 GW_ADDR 是多余的，因为 (DST_NET + DST_MASK) 组合就是一条静态路由的主键，如果你真想这么做，就需要做语法拆分，独立注册一条  UNDO_FLAG 的语法，如下所示。
    ```c
    add_cmd_easily(cmd_tree, "route DST_NET DST_MASK GW_ADDR", CONFIG_VIEW, DO_FLAG);
    add_cmd_easily(cmd_tree, "route DST_NET DST_MASK", CONFIG_VIEW, UNDO_FLAG);
    ```
## 4.3 特殊语法字符 [ ] { | } 的使用以及限制

Libocli 注册语法时可使用 **[ ] { | }** 表达选项语法：
- **多选一** 语法段 **{ | }**  ，每个符号单词之间必须使用 **|** 分隔，比如 " { block | pass } "，" { tcp | udp | icmp } "
- **可选项** 语法段 **[  ]**，一个段落内可包含多个符号单词，比如 " [ -c COUNT ] [ -s PKT_SIZE ] "

使用限制注意：
- 特殊字符之间，以及特殊字符与其他符号单词之间，必须用空格隔离
- 多选一段落 **{ }** 之内不允许再嵌套任何 **[ ]** 或 **{ }**
- 可选项段落 **[ ]** 内允许嵌套一层多选一 **{ }**，比如 " [ from { IP_ADDR | IFNAME } ] "


## 4.4 添加个性化手册文本

如果你不想使用 add_cmd_easily() 自动生成的手册，而需要个性化创建手册，那么可以调用 add_cmd_manual() 接口，再调用 add_cmd_syntax()。add_cmd_manual() 接口定义如下： 

```c
int add_cmd_manual(struct cmd_tree *cmd_tree,	/* 语法树指针 */
		   char *text,			/* 手册文本 */
		   int view_mask		/* 本手册的视图掩码 */*
		   );
```

当使用 man 查看一条命令的手册时，Libocli 会按手册文本添加的顺序，逐行显示手册内容。
 
