# 4. Libocli 命令行语法注册接口

中文 | [English](Syntax%20Register.md)
<br>
[<< 上一级目录](README.zh_CN.md)  

作者：Digger Wu (digger.wu@linkbroad.com)

## 4.1 创建命令

创建命令接口函数为 create_cmd_tree()，定义如下：

```
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

SYM_TABLE 宏可以简化调用写法，如 [netutils.c](../example/netutils.c) 所示：
```
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
- 第二个参数是个整型，例子中的回调函数都使用 do_flag 来命名这个参数。这个参数用于告知回调函数当下这个命令常规执行的（即 do_flag| DO_FLAG 为真），还是以 "no" 语法方式执行的（即 do_flag | UNDO_FLAG 为真）。很明显 ping 命令不需要 no 语法，但是配置命令可能需要，比如例子中的 route 命令，删除路由时需要 no route ...，参考 [route.c](../example/route.c)。

