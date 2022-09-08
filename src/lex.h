/*
 *  libocli, A general C library to provide a open-source cisco style
 *  command line interface.
 *
 *  Copyright (C) 2015-2022 Digger Wu (digger.wu@linkbroad.com)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * lex.h, the lexical parsing header
 */

#ifndef _OCLI_LEX_H
#define _OCLI_LEX_H

#include <sys/types.h>
#include <string.h>
#include <pcre.h>
#include <netinet/in.h>

typedef int (*lex_fun_t)(char *);

#define	LEX_NAME_LEN	20
#define	LEX_TEXT_LEN	80

/* lexical entry */
struct lex_ent {
	char	name[LEX_NAME_LEN];	/* an all capital name */
	lex_fun_t fun;			/* parsing function */
	char	help[LEX_TEXT_LEN];	/* lexical help text */
	char	prefix[LEX_TEXT_LEN];	/* prefix, eth, tun */
};

typedef enum lex_type {
	LEX_IP_ADDR,
	LEX_IP_MASK,
	LEX_IP_PREFIX,
	LEX_IP_BLOCK,
	LEX_IP_RANGE,
	LEX_IP6_ADDR,
	LEX_IP6_PREFIX,
	LEX_IP6_BLOCK,
	LEX_PORT,
	LEX_PORT_RANGE,
	LEX_VLAN_ID,
	LEX_MAC_ADDR,
	LEX_WORD,
	LEX_WORDS,
	LEX_INT,
	LEX_HEX,
	LEX_DECIMAL,
	LEX_HOST_NAME,
	LEX_HOST,
	LEX_HOST6,
	LEX_DOMAIN_NAME,
	LEX_EMAIL_ADDR,
	LEX_HTTP_URL,
	LEX_HTTPS_URL,
	LEX_FTP_URL,
	LEX_SCP_URL,
	LEX_TFTP_URL,
	LEX_FILE_NAME,
	LEX_FILE_PATH,
	LEX_UID,
	LEX_NET_UID,
	LEX_NET6_UID,
	LEX_DATE_TIME,
	LEX_CUSTOM_BASE_TYPE	/* customized type starts here */
} lex_type_t;

#define MAX_CUSTOM_LEX_NUM	128

#define LEX_CUSTOM_TYPE(x) (LEX_CUSTOM_BASE_TYPE + x)

#define MAX_LEX_TYPE (LEX_CUSTOM_BASE_TYPE + MAX_CUSTOM_LEX_NUM)

#define IS_CUSTOM_LEX_TYPE(type) \
	(type >= LEX_CUSTOM_BASE_TYPE && type < MAX_LEX_TYPE)

#define IS_VALID_LEX_TYPE(type) \
	(type >= 0 && type < MAX_LEX_TYPE)

#define IS_NUMERIC_LEX_TYPE(type) \
	(type == LEX_INT || type == LEX_DECIMAL)

/*
 * module funcs
 */
extern int lex_init(void);
extern void lex_exit(void);
extern struct lex_ent *get_lex_ent(int type);
extern struct lex_ent *get_lex_ent_by_name(char *name);

extern int pcre_custom_match(char *str, int idx, char *pattern);
extern int set_custom_lex_ent(int type, char *name, lex_fun_t fun, char *help, char *prefix);

/*
 * paring funcs, return TRUE (1) if matched, else return FALSE (0)
 */ 
extern int is_ip_addr(char *str);
extern int is_ip_mask(char *str);
extern int is_ip_prefix(char *str);
extern int is_ip_block(char *str);
extern int is_ip_range(char *str);

extern int is_ip6_addr(char *str);
extern int is_ip6_prefix(char *str);

extern int is_vlan_id(char *str);
extern int is_port(char *str);
extern int is_port_range(char *str);

extern int is_host_name(char *str);
extern int is_host(char *str);
extern int is_host6(char *str);
extern int is_domain_name(char *str);
extern int is_domain_wildcard(char *str);
extern int is_email_addr(char *str);
extern int is_http_url(char *str);
extern int is_https_url(char *str);
extern int is_ftp_url(char *str);
extern int is_scp_url(char *str);
extern int is_tftp_url(char *str);

extern int is_int(char *str);
extern int is_hex(char *str);
extern int is_decimal(char *str);
extern int is_vlan_id(char *str);
extern int is_mac_addr(char *str);

extern int is_word(char *str);
extern int is_words(char *str);
extern int is_http_url(char *str);
extern int is_https_url(char *str);
extern int is_ftp_url(char *str);

extern int is_uid(char *str);
extern int is_net_uid(char *str);

extern int is_date_time(char *str);

extern int is_empty_line(char *str);

/*
 * string funcs
 */
extern in_addr_t bits_to_netmask(int bits);
extern int netmask_to_bits(in_addr_t s_addr);
extern int get_subnet_mask(char *str, struct in_addr *ia_net, struct in_addr *ia_mask, int fix_net);
extern int get_ip6_addr_pfx(char *str, struct in6_addr *ia6, int *pfx_len);
extern int get_binary_mac(char *str, u_char *mac, int len);
extern int get_formal_mac(char *str, char *mac_addr, int len);
extern int get_ip_range(char *str, struct in_addr *ia_from, struct in_addr *ia_to);
extern int get_port_range(char *str, u_short *port_from, u_short *port_to);
extern int get_uri_elements(char *str, char **pproto, char **phost, char **ppath, char **pfile);

#endif
