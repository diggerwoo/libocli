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
	LEX_IP4_IP6_ADDR,
	LEX_IP6_PREFIX,
	LEX_IP6_BLOCK,
	LEX_IFNAME_ADDR,
	LEX_PORT,
	LEX_PORT_RANGE,
	LEX_VLAN_ID,
	LEX_MAC_ADDR,
	LEX_WORD,
	LEX_WORDS,
	LEX_PARAM,
	LEX_INT,
	LEX_HEX,
	LEX_DECIMAL,
	LEX_HOST_NAME,
	LEX_HOST,
	LEX_HOST6,
	LEX_DOMAIN_NAME,
	LEX_DOMAIN_WILDCARD,
	LEX_DOMAIN_EXT,
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
	LEX_RADIUS_UID,
	LEX_CLEAR_PASSWD,
	LEX_MD5_CIPHER,
	LEX_DATE_TIME,
	LEX_DATE,
	LEX_ETH_IFNAME,
	LEX_TUN_IFNAME,
	LEX_PPP_IFNAME,
	LEX_MBITS_BW,
	MAX_LEX_TYPE
} lex_type_t;

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

/*
 * paring funcs, return TRUE (1) if matched, else return FALSE (0)
 */ 
extern int is_ip_addr(char *str);
extern int is_ip_mask(char *str);
extern int is_ip_prefix(char *str);
extern int is_ip_block(char *str);
extern int is_ip_range(char *str);

extern int is_ip6_addr(char *str);
extern int is_ip4_ip6_addr(char *str);
extern int is_ip6_prefix(char *str);
extern int is_ifname_addr(char *str);

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
extern int is_param(char *str);
extern int is_http_url(char *str);
extern int is_https_url(char *str);
extern int is_ftp_url(char *str);

extern int is_uid(char *str);
extern int is_net_uid(char *str);
extern int is_radius_uid(char *str);
extern int is_clear_passwd(char *str);
extern int is_md5_cipher(char *str);

extern int is_date_time(char *str);

extern int is_fw_oper(char *str);
extern int is_pkt_dir(char *str);
extern int is_nat_oper(char *str);
extern int is_proto(char *str);
extern int is_str_filter(char *str);
extern int is_empty_line(char *str);

extern int is_eth_ifname(char *str);
extern int is_xctl_ifname(char *str);
extern int is_tun_ifname(char *str);
extern int is_ppp_ifname(char *str);
extern int get_max_ifindex(char *prefix);

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
extern char *get_full_lex_string(int type, char *str);

#endif
