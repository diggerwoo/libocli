# 3. Libocli 可扩展的词法分析接口

中文 | [English](Lexical%20Parsing.md)
<br>
[<< 上一级目录](README.zh_CN.md)  

作者：Digger Wu (digger.wu@linkbroad.com)

## 3.1 词法类型和词法分析函数

Libocli 在 [lex.h](../src/lex.h) 中定义了网络管理领域常用的词法类型 ID，以及可供调用的词法分析函数（多数函数基于 pcre 实现）。词法分析函数统一为 int is_xxx(str) 命名，参数为字符串指针，匹配则返回真。

| 词法类型 | 说明 | 词法分析函数 |
| :--- | :--- | :--- |
| LEX_INT | 10进制整型 | is_int() |
| LEX_HEX | 16进制整型 | is_hex() |
| LEX_IP_ADDR | IPv4 地址 | is_ip_addr() |
| LEX_IP_MASK | IPv4 掩码 | is_ip_mask() |
| LEX_IP_PREFIX | IPv4Addr/<0-32> 即前缀式子网 | is_prefix() |
| LEX_IP_BLOCK | IPv4Addr[/<0-32>] | is_ip_block() |
| LEX_IP_RANGE | IPAddr1[-IPAddr2] IP地址范围 | is_ip_range() |
| LEX_IP6_ADDR | IPv6Addr | is_ip6_addr() |
| LEX_IP6_PREFIX | IPv6Addr/<0-128> 前缀式子网 | is_ipv6_prefix() |
| LEX_IP6_BLOCK | IPv6Addr[/<0-128>] | is_ipv6_block() |
| LEX_PORT | <0-65535> TCP/UDP Port | is_port() |
| LEX_PORT_RANGE | Port1[-Port2] | is_port_range() |
| LEX_VLAN_ID | <1-4094> VLAN ID | is_vlan_id() |
| LEX_MAC_ADDR | MAC 地址 | is_mac_addr() |
| LEX_HOST_NAME | 主机名或域名 | is_host_name() |
| LEX_DOMAIN_NAME | 域名 | is_domain_name() |
| LEX_HTTP_URL | HTTP URL | is_http_url() |
| LEX_HTTPS_URL | HTTPS URL | is_https_url() |
| LEX_EMAIL_ADDR | EMAIL 地址 | is_email_url() |
| LEX_UID | 字母数字_.的用户名 | is_uid() |
| LEX_WORD | 字母开始词 | is_word() |
| LEX_WORDS | 任意串 | is_words() |

## 3.2 自定义词法接口

Libocli 支持自定义词法，最多可以扩展 128 个自定义词法。宏 LEX_CUSTOM_TYPE(x) 可用于创建自定义词法类型 ID，其中参数 x 的范围为 0 ~ 127。相关的宏定义请参考 [lex.h](../src/lex.h)。

自定义词法时需要调用词法注册函数 set_custom_lex_ent() ，函数接口具体定义如下：
```c
/* 成功返回 0，同一个词法类型 ID 不允许重注册 */
int set_custom_lex_ent(int type,       /* 词法类型 ID */
                       char *name,     /* 词法类型的内部可读名称 */
                       lex_fun_t fun,  /* 词法解析函数指针 */
                       char *help,     /* 词法的帮助文本，供命令行 '?' 键查看 */
                       char *prefix    /* 词法的固定前缀串，供命令行 TAB 键自动补齐 */
                       );
```

Libocli 还提供了一个封装的正则匹配函数 pcre_custom_match()，提供正则匹配功能，并对降低 pcre_compile() 的调用频次做了优化。
```c
/* 
 * 匹配成功返回 1，无匹配返回 0，出错返回 -1 。
 *
 * idx 为自定义词法的 ID，同时也是 Libocli 的正则 cache 表的索引 ID 。
 * 当正则表达式被调用一次后，就在 idx 索引位置创建正则 Cache 保存 pcre_compile() 返回，供下次 pcre_exec() 直接使用，
 * 而不是每次 pcre_exec() 之前都重复 pcre_compile() 。
 */
int pcre_custom_match (char *str,       /* 字符串 */
                       int idx,         /* 自定义词法类型 ID */
                       char *pattern    /* 正则表达式 */
                       );
```

除非一个词法确实有固定的前缀串可供 TAB 补齐，否则调用时应将 prefix 设为 NULL。某些 URL 类词法需要 prefix，比如 Libocli 自带的词法 LEX_HTTP_URL 前缀是 "http://" ，LEX_HTTPS_URL 前缀是 "https://" 。其它可能的使用场景是网络接口名字，比如自定义一个 Linux 风格的以太网接口词法，其命名规则为 "eth<0-9>"，那么可以指定 prefix 参数为 "eth"。democli 的 [mylex.c](../example/mylex.c) 中提供了一个范例，使用了上述接口函数创建了自定义 ETH_IFNAME 词法，其中网口索引的最大值是读取 Linux /proc 自动生成的。有兴趣可在阅读完 3.3 章节后参考。

## 3.3 自定义词法举例

假设你需要增加两个词法，LEX_FOO_0 和 LEX_FOO_1，建议的步骤如下：

1. 在自己的头文件，比如 mylex.h 中定义新增的词法和解析函数：
    ```c
    #include <ocli/lex.h>

    /* 增加两个自定义词法类型 */
    #define LEX_FOO_0 LEX_CUSTOM_TYPE(0)
    #define LEX_FOO_1 LEX_CUSTOM_TYPE(1)

    /* 词法分析函数 */
    extern int is_foo_0(char *str);
    extern int is_foo_1(char *str);

    /* 自定义词法初始化函数 */
    int mylex_init();
    ```

2. 在自定义词法分析模块，比如 mylex.c 中实现上述函数：
    ```c
    #include "mylex.h"

    int is_foo_0(char *str)
    {
            /* ... */
            return 1;
    }

    int is_foo_1(char *str)
    {
            /* ... */
            return 1;
    }

    int mylex_init()
    {
            set_custom_lex_ent(LEX_FOO_0, "FOO 0", is_foo_0, "Help for my foo 0", NULL);
            set_custom_lex_ent(LEX_FOO_1, "FOO 1", is_foo_1, "Help for my foo 1", NULL);
    }
    ```

3. 主程序初始化 libocli_rl_init() 后，调用 mylex_init()，注册上述自定义词法
4. 之后各个模块都可以使用 LEX_FOO_0 和 LEX_FOO_1 来定义自己的符号词法类型了，注意不要忘记 #include "mylex.h"

