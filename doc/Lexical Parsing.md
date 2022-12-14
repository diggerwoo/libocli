# 3. Extensible Lexical Parsing Interface

English | [中文](Lexical%20Parsing.zh_CN.md)
<br>
[<< Upper Level](README.md)  

Author: Digger Wu (digger.wu@linkbroad.com)

## 3.1 Lexical types and parsing functions

Libocli defines lexical types which are commonly used for network adminitration in [lex.h](../src/lex.h), and provides coreponding parsing function for eache lexical type. The parsing functions are all defined as **int is_xxx(char *str)**, which returns True(1) if parsing successful.

| Lexical Type | Description | Parsing Function |
| :--- | :--- | :--- |
| LEX_INT | Integer | is_int() |
| LEX_HEX | Hexidecimal | is_hex() |
| LEX_IP_ADDR | IPv4 address | is_ip_addr() |
| LEX_IP_MASK | IPv4 mask | is_ip_mask() |
| LEX_IP_PREFIX | IPv4Addr/<0-32> | is_prefix() |
| LEX_IP_BLOCK | IPv4Addr[/<0-32>] | is_ip_block() |
| LEX_IP_RANGE | IPAddr1[-IPAddr2] | is_ip_range() |
| LEX_IP6_ADDR | IPv6Addr | is_ip6_addr() |
| LEX_IP6_PREFIX | IPv6Addr/<0-128> | is_ipv6_prefix() |
| LEX_IP6_BLOCK | IPv6Addr[/<0-128>] | is_ipv6_block() |
| LEX_PORT | <0-65535> TCP/UDP Port | is_port() |
| LEX_PORT_RANGE | Port1[-Port2] | is_port_range() |
| LEX_VLAN_ID | <1-4094> VLAN ID | is_vlan_id() |
| LEX_MAC_ADDR | MAC address | is_mac_addr() |
| LEX_HOST_NAME | Hostname or domain name| is_host_name() |
| LEX_DOMAIN_NAME | Domain name | is_domain_name() |
| LEX_HTTP_URL | HTTP URL | is_http_url() |
| LEX_HTTPS_URL | HTTPS URL | is_https_url() |
| LEX_EMAIL_ADDR | EMail address | is_email_url() |
| LEX_UID | User ID | is_uid() |
| LEX_WORD | Word | is_word() |
| LEX_WORDS | Any string | is_words() |

## 3.2 Customized lexical type

Libocli supports up to 128 customized lexical types. The macro LEX_CUSTOM_TYPE(x) is used to define a customized lexical type ID, where the x ranges from 0 to 127. For related macro definitions, please refer to [lex.h](../src/lex.h).


The function set_custom_lex_ent() is used to register a customized lexical type, which is defined as blow:
```c
/* Returns 0 on success. The same type ID is not allowed to register repeatedly */
int set_custom_lex_ent(int type,       /* Lexical type ID */
                       char *name,     /* Readable name of lexical type */
                       lex_fun_t fun,  /* Parsing function */
                       char *help,     /* Help text for '?' key stroke */
                       char *prefix    /* Prefix string for TAB auto completion */
                       );
```

Unless a lexical type does have a fixed prefix string for the TAB auto completion, should the prefix parameter be set to NULL. Some Libocli builtin URLs do have prefixes. For example, the LEX_HTTP_URL has prefix "http://" , and the LEX_HTTPS_URL has prefix "https://" . Other possible use cases are network interface names. E.g. the naming scheme of a Ethernet interface is "eth<0-9>", then the prefix should be specified as "eth".

Libocli also provides a PCRE matching function pcre_custom_match(), which encapsulates the libpcre functions with Cache optimization to reduce the calling frequency of pcre_compile().
```c
/* 
 * Return value: 1 if str matches pattern, 0 if unmatched, -1 error ocurrs.
 *
 * The index is the lexical type ID，it is also the row ID of the PCRE cache table.
 * Once the function is called, a cache is created at the index position to save the 
 * result of pcre_compile() for direct use by the next pcre_exec() call, instead of
 * calling pcre_compile() repeatedly before each pcre_exec().
 */
int pcre_custom_match (char *str,       /* String to match */
                       int index,       /* Customized lexical type ID */
                       char *pattern    /* Regular expression */
                       );
```

## 3.3 How to customize

For example you need to add two customized lexical types, LEX_FOO_0 和 LEX_FOO_1. The suggested steps will be:

1. Define the lexical types and parsing functions in your own header file, e.g. mylex.h:
    ```c
    #include <ocli/lex.h>

    /* Add two customized lexical types */
    #define LEX_FOO_0 LEX_CUSTOM_TYPE(0)
    #define LEX_FOO_1 LEX_CUSTOM_TYPE(1)

    /* Parsing functions */
    extern int is_foo_0(char *str);
    extern int is_foo_1(char *str);

    /* Init function to register your cutomized types */
    int mylex_init();
    ```

2. Implement the functions, e.g. in mylex.c:
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

3. Integrate with main(): Call mylex_init() after libocli_rl_init().
4. Now your can define variable symbols with newly customized types LEX_FOO_0 and LEX_FOO_1 in other modules. Don't forget to #include "mylex.h".

There is a more complex use case in the democli's [mylex.c](../example/mylex.c) which implements a customized lexical type ETH_IFNAME. Then in [interface.c](../example/interface.c) a Cisco-like command "interface ETH_IFNAME" is created based on this cutomized lexical type. E.g.  user inputs "interface eth0" to enter the interface configuration view, in which IP address of eth0 can be configured by the "ip address" command.
