# 3. Libocli 词法分析

中文 | [English](Lexical%20Parsing.md)
<br>
[<< 上一级目录](README.zh_CN.md)  

作者：Digger Wu (digger.wu@linkbroad.com)

## 3.1 词法类型和解析函数

Libocli 在 [lex.h](../src/lex.h) 中定义了常用的词法，用于定义变量类型符号的词法类型。每个词法类型对应一个词法解析函数（基于 pcre 实现）可直接用于判断词法，词法解析函数全部是 int is_xxx(str) 风格命名，参数为字符串指针，匹配则返回真。

| 词法类型 | 说明 | 词法解析函数 |
| :--- | :--- | :--- |
| LEX_INT | 10进制整型 | is_int() |
| LEX_HEX | 16进制整型 | is_hex() |
| LEX_IP_ADDR | IPv4 地址 | is_ip_addr() |
|	LEX_IP_MASK | IPv4 掩码 | is_ip_mask() |
| LEX_IP_PREFIX | IPv4Addr/<0-32> 即前缀式子网 | is_prefix() |
| LEX_IP_BLOCK | IPv4Addr[/<0-32>] | is_ip_block() |
| LEX_IP_RANGE | IPAddr1[-IPAddr2] IP地址范围 | is_ip_range() |
| LEX_IP6_ADDR | IPv6Addr | is_ip6_addr() |
|	LEX_IP6_PREFIX | IPv6Addr/<0-128> 前缀式子网 | is_ipv6_prefix() |
| LEX_IP6_BLOCK | IPv6Addr[/<0-128>] | is_ipv6_block() |
| LEX_PORT | <0-65535> TCP/UDP Port | is_port() |
|	LEX_PORT_RANGE | Port1[-Port2] | is_port_range() |
|	LEX_VLAN_ID | <1-4094> VLAN ID | is_vlan_id() |
|	LEX_MAC_ADDR | MAC 地址 | is_mac_addr() |
| LEX_HOST_NAME | 主机名或域名 | is_host_name() |
|	LEX_DOMAIN_NAME | 域名 | is_domain_name() |
| LEX_HTTP_URL | HTTP URL | is_http_url() |
| LEX_HTTPS_URL | HTTPS URL | is_https_url() |
| LEX_EMAIL_ADDR | EMAIL 地址 | is_email_url() |
| LEX_UID | 字母数字_.的用户名 | is_uid() |
| LEX_WORD | 字母开始词 | is_word() |
| LEX_WORDS | 任意串 | is_words() |



