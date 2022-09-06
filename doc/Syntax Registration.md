# 4. Command and Syntax Registration

English | [中文](Syntax%20Registration.zh_CN.md)
<br>
[<< Upper Level](README.md)  

Author: Digger Wu (digger.wu@linkbroad.com)

## 4.1 Create a command

Function create_cmd_tree() is used to create a command. Below is the specification.

```c
#define SYM_NUM(syms) (sizeof(syms)/sizeof(symbol_t))
#define SYM_TABLE(syms) &syms[0], SYM_NUM(syms)

/* Callback definition */
typedef int (*cmd_fun_t)(cmd_arg_t *, int);

/*
 * Returns struct cmd_tree * pointer, or NULL on failure.
 */
struct cmd_tree *
create_cmd_tree(char *cmd,              /* Command keyword */
                symbol_t *sym_table,    /* Pointer to symbol table */
                int sym_num,            /* Number of symbols in the table */
                cmd_fun_t fun           /* Callback function */
                );
```

The SYM_TABLE macro can be used to simplify create_cmd_tree() calls, as shown in [netutils.c](../example/netutils.c):
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

The callback function type **cmd_fun_t** has two parameters:
- The first is a pointer of type **cmd_arg_t**, which points to the array of callback argument. For details please refer to [Callback Argument section 2.2](Symbol%20Definition.md#22-passing-callback-arguments). 
- The second is a integer that we always use "do_flag" to represent. It is related to the "no" syntax. If the command is started with "no", then (do_flag & UNDO_FLAG) will be True, otherwise the (do_flag & DO_FLAG) will be Ture. Apparently the "ping" command doesn't have the "no" syntax. whilst other configuration commands like the "route" need this. We use "route ..." to add a static route, also use "no route ..." to delete one. Please refer to [route.c](../example/route.c).

## 4.2 Register a syntax

Functions add_cmd_easily() and add_cmd_syntax() can all be used to register a syntax.
```c
/* Predefined VIEW, each coresponds to one bit */
#define	BASIC_VIEW		0x0001
#define	ENABLE_VIEW		0x0002
#define	CONFIG_VIEW		0x0004

/* Bit definition of do_flag. */
#define	DO_FLAG		0x01
#define	UNDO_FLAG	0x02

/* Register a syntax, and create the manual line for this syntax. Returns 0 on success, else -1 */
int add_cmd_easily(struct cmd_tree *cmd_tree,   /* Pointer returned by create_cmd_tree() */
                   char *syntax,                /* Syntax string */
                   int view_mask,               /* VIEW mask, can be OR result of multi VIEW */
                   int do_flag                  /* OR result of DO_FLAG, UNDO_FLAG */
                   );

/* Register a syntax only. Returns 0 on success, else -1 */
int add_cmd_syntax(struct cmd_tree *cmd_tree,
                   char *syntax,
                   int view_mask,
                   int do_flag);

```

We give several typical sytax registration examples as below.

1. Register a "ping" syntax. As done in [netutils.c](../example/netutils.c), this syntax can be accessed in all VIEW except the BASIC_VIEW and dosen't support "no" usage, so the last parameter is DO_FLAG.
    ```c
    add_cmd_easily(cmd_tree, "ping [ -c COUNT ] [ -s SIZE ] { HOST | HOST_IP } [ from IFADDR ]",
                   (ALL_VIEW_MASK & ~BASIC_VIEW), DO_FLAG);
    ```

2. Register "enable" and "enable password" syntaxes in different VIEWs, as done in [democli.c](../example/democli.c).
    ```c
    /* Reigster "enable" in BASIC_VIEW, which is used to input the enabled password */
    add_cmd_easily(cmd_tree, "enable", BASIC_VIEW, DO_FLAG);

    /* Register "enable password" in ENABLE_VIEW, which is used to update the enabled password */
    add_cmd_easily(cmd_tree, "enable password", ENABLE_VIEW, DO_FLAG);
    ```

3. Register the "[no] route" syntax. Now the do_flag is set to (DO_FLAG | UNDO_FLAG). This is a configuration command so it is only accessible in CONFIG_VIEW. Refer to [route.c](../example/route.c).
    ```c
    add_cmd_easily(cmd_tree, "route DST_NET DST_MASK GW_ADDR", CONFIG_VIEW, (DO_FLAG | UNDO_FLAG));
    ```

4. For the previous example, you might think the GW_ADDR is redundant when you use "no route" to delete static route, because (DST_NET + DST_MAKE) is actually the primary key of the route table. If you really want to optimize this, you need to do syntax splitting and register an UNDO_FLAG syntax independently, as shown below.
    ```c
    add_cmd_easily(cmd_tree, "route DST_NET DST_MASK GW_ADDR", CONFIG_VIEW, DO_FLAG);
    add_cmd_easily(cmd_tree, "route DST_NET DST_MASK", CONFIG_VIEW, UNDO_FLAG);
    ```
## 4.3 Usage and limitation of reserved syntax chars [ ] { | }

Libocli allows to use **[ ] { | }** for optional / alternative segments when registering syntax:
- **Alternative** segment **{ | }**  , allows two of more tokens separated by '**|**' . E.g. " { block | pass } "，" { tcp | udp | icmp } "
- **Optional** segment **[  ]**, each segment can have multiple tokens. E.g. " [ -c COUNT ] [ -s PKT_SIZE ] "

Limitaions:
- SPACE must be present between reserved chars, or between reserved char and other tokens.
- NO **[ ]** or **{ }** are allowed to be nested inside **{ }**.
- **{ }** can be nested inside **[ ]**. E.g.  " [ from { IP_ADDR | IFNAME } ] "


## 4.4 Add customized manual

If you do not want to use the manual generated automatically by add_cmd_easily(), but need to create a customized one, you can call the add_cmd_manual(), then call add_cmd_syntax(). The function add_cmd_manual() is defined as below.

```c
int add_cmd_manual(struct cmd_tree *cmd_tree,	/* Pointer to command tree */
		   char *text,			/* Manual text */
		   int view_mask		/* VIEW mask */*
		   );
```

When using "man" to read the manual, Libocli displays the manual text line by line, in the order in which the manual text was added.
 
