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
 * lex.c, the lexical parsing module of libocli
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <pcre.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lex.h"

static int	lex_init_ok = 0;

/* pcre precompile local cache */
static pcre	*pcre_cache[MAX_LEX_TYPE];

#define	OVECSIZE	30	/* man pcre said: must be multiple of 3 */

static struct lex_ent lex_ent[MAX_LEX_TYPE];

/*
 * pcre match function
 */
static int
pcre_match(char *str, int idx, char *pattern)
{
	static pcre *pcre = NULL;
	const char *errptr = NULL;
	int	erroffset = 0;
	int	ovector[OVECSIZE];
	int	res;
	
	if (!str || !str[0] || !pattern || !pattern[0])
		return 0;

	/* try cache before compile */
	if (idx >= 0 && idx < MAX_LEX_TYPE && pcre_cache[idx] != NULL) {
		pcre = pcre_cache[idx];

	} else {
		pcre = pcre_compile(pattern, 0, &errptr, &erroffset, NULL);
		if (pcre == NULL) {
			fprintf(stderr, "pcre_comile: %s\n", errptr);
			return -1;
		}
		/* store it if never cache before */
		if (idx >= 0 && idx < MAX_LEX_TYPE && pcre_cache[idx] == NULL) {
			pcre_cache[idx] = pcre;
		}
	}

			    
	res = pcre_exec(pcre, NULL, str, strlen(str), 0, 0, ovector, OVECSIZE);

	/* out of cache range, release it immediately */
	if (idx < 0 || idx >= MAX_LEX_TYPE) {
		pcre_free(pcre);
	}

	return (res >= 0);
}

/*
 * is str an ipv4 address ?
 */
int
is_ip_addr(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_IP_ADDR,
			 "^(([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.){3}"
			 "([01]?\\d\\d?|2[0-4]\\d|25[0-5])$");
	return (res == 1);
}

/*
 * is str an ipv4 mask ?
 */
int
is_ip_mask(char *str)
{
	u_int	ia;
	u_int	m = 0xffffffff;	/* init as a all one mask */
	int	i;

	if (!is_ip_addr(str)) return 0;

	ia = ntohl(inet_addr(str));

	if (ia == m) return 1;

	for (i = 0; i < 32; i++) {
		m <<= 1;
		if (ia == m) return 1;
	}

	return 0;
}

/*
 * is str an ipv4 with mask bit prefix? ip/mask_bits 
 */
int
is_ip_prefix(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_IP_PREFIX,
			 "^(([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.){3}"
			 "([01]?\\d\\d?|2[0-4]\\d|25[0-5])"
			 "(/[0-2]?\\d|/3[0-2])$");
	return (res == 1);
}

/*
 * is str an ipv4 network block ? net[/mask_bits]
 */
int
is_ip_block(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_IP_BLOCK,
			 "^(([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.){3}"
			 "([01]?\\d\\d?|2[0-4]\\d|25[0-5])"
			 "(/[0-2]?\\d|/3[0-2])?$");
	return (res == 1);
}

/*
 * is str an ipv4 range ? ip1[-ip2]
 */
int
is_ip_range(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_IP_RANGE,
			 "^(([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.){3}"
			 "([01]?\\d\\d?|2[0-4]\\d|25[0-5])"
			 "(\\-(([01]?\\d\\d?|2[0-4]\\d|25[0-5])\\.){3}"
			 "([01]?\\d\\d?|2[0-4]\\d|25[0-5]))?$");
	return (res == 1);
}

/*
 * is str an ipv6 address ?
 */
int
is_ip6_addr(char *str)
{
	struct in6_addr ia6;

	if (!str || !str[0]) return 0;
	return (inet_pton(AF_INET6, str, &ia6) == 1);
}

/*
 * is str an ipv4 or ipv6 address
 */
int
is_ip4_ip6_addr(char *str)
{
	return (is_ip_addr(str) || is_ip6_addr(str));
}

/*
 * is interface name or ipv4/ipv6 address
 */
int
is_ifname_addr(char *str)
{
	return (is_ip4_ip6_addr(str) || is_eth_ifname(str) ||
		is_tun_ifname(str) || is_ppp_ifname(str));
}

/*
 * is str an ipv6 prefix ? 
 */
int
is_ip6_prefix(char *str)
{
	char	addr6[INET6_ADDRSTRLEN], *slash;
	int	len;

	if (!str || !str[0]) return 0;

	if ((slash = strchr(str, '/')) == NULL) return 0;
	if (!isdigit(*(slash + 1))) return 0;

	bzero(addr6, sizeof(addr6));
	strncpy(addr6, str, slash - str);

	if (!is_ip6_addr(addr6) || !is_int(slash + 1)) return 0;

	len = atoi(slash + 1);

	return (len >= 0 && len <= 128);
}

/*
 * is str an ipv6 ACL block ? 
 */
int
is_ip6_block(char *str)
{
	return (is_ip6_addr(str) || is_ip6_prefix(str));
}

/*
 * parse and get back ipv6 address and prefix info
 */
int
get_ip6_addr_pfx(char *str, struct in6_addr *ia6, int *pfx_len)
{
	char	addr6[INET6_ADDRSTRLEN], *slash;

	if (!str || !ia6 || !pfx_len) return 0;
	if (!is_ip6_block(str)) return 0;

	if ((slash = strchr(str, '/')) != NULL) {
		bzero(addr6, sizeof(addr6));
		strncpy(addr6, str, slash - str);
		inet_pton(AF_INET6, addr6, ia6);
		*pfx_len = atoi(slash + 1);
	} else {
		inet_pton(AF_INET6, str, ia6);
		*pfx_len = 128;
	}

	return 1;
}

/*
 * is str an host name ?
 */
int
is_host_name(char *str)
{
	int	res;

	if (!str || !str[0] || is_ip_addr(str)) return 0;

	/*
	 * host name with optional domain
	 * if domain presents, the top layer domain must be all alphabets
	 */
	res = pcre_match(str, LEX_HOST_NAME,
			 "^(\\w[\\w\\-]*)"
			 "((\\.\\w[\\w\\-]*)*(\\.[A-Za-z]+))*$");

	return (res == 1);
}

/*
 * is str an host name or IP ?
 */
int
is_host(char *str)
{
	return (is_ip_addr(str) || is_host_name(str));
}

/*
 * is str an host name or IP4/6 address ?
 */
int
is_host6(char *str)
{
	return (is_host(str) || is_ip6_addr(str));
}

/*
 * is str an domain name ?
 */
int
is_domain_name(char *str)
{
	int	res;

	if (!str || !str[0] || is_ip_addr(str)) return 0;

	/*
	 * host name with at least one layer domain
	 * XXX the top layer domain must be all alphabets
	 */
	res = pcre_match(str, LEX_DOMAIN_NAME,
			 "^(\\w[\\w\\-]*\\.)+"
			 "([A-Za-z0-9]+)$");

	return (res == 1);
}

/*
 * is str an domain wild-card ?
 */
int
is_domain_wildcard(char *str)
{
	int	res;

	if (!str || !str[0] || is_ip_addr(str)) return 0;

	/*
	 * domain name begin with '*.'
	 * XXX the top layer domain must be all alphabets
	 */
	res = pcre_match(str, LEX_DOMAIN_WILDCARD,
			 "^(\\*\\.)(\\w[\\w\\-]*\\.)+"
			 "([A-Za-z0-9]+)$");

	return (res == 1);
}

/*
 * is str a domain name or domain wildcard ?
 */
int
is_domain_ext(char *str)
{
	return (is_domain_name(str) || is_domain_wildcard(str));
}

/*
 * is str an email address ?
 */
int
is_email_addr(char *str)
{
	int	res;

	if (!str || !str[0] || is_ip_addr(str)) return 0;

	res = pcre_match(str, LEX_EMAIL_ADDR,
			 "^(\\w[\\w\\-\\.]*@)"
			 "(\\w[\\w\\-]*\\.)+"
			 "([A-Za-z]+)$");

	return (res == 1);
}

/*
 * is str an integer ?
 */
int
is_int(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_INT, "^(\\d+)$");

	return (res == 1);
}

/*
 * is str an hexadicimal ?
 */
int
is_hex(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_HEX, "^(0[xX])?([\\da-fA-F]{1,8})$");

	return (res == 1);
}

/*
 * is str an decimal ?
 */
int
is_decimal(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_DECIMAL, "^(\\d+)(\\.\\d*)?$");

	return (res == 1);
}

/*
 * is str a layer-4 TCP/UDP port number <0-65535> ?
 */
int
is_port(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_PORT,
			 "^(([0-9]{1,4})|([0-5][0-9]{1,4})|"
			 "(6[0-4][0-9][0-9][0-9])|(65[0-4][0-9][0-9])|"
			 "(655[0-2][0-9])|(6553[0-5]))$");

	return (res == 1);
}

/*
 * is str a layer-4 TCP/UDP port range port1[-port2] ?
 */
int
is_port_range(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_PORT_RANGE,
			 "^(([0-9]{1,4})|([0-5][0-9]{1,4})|"
			 "(6[0-4][0-9][0-9][0-9])|(65[0-4][0-9][0-9])|"
			 "(655[0-2][0-9])|(6553[0-5]))"

			 "(-(([0-9]{1,4})|([0-5][0-9]{1,4})|"
			 "(6[0-4][0-9][0-9][0-9])|(65[0-4][0-9][0-9])|"
			 "(655[0-2][0-9])|(6553[0-5])))?$");

	return (res == 1);
}

/*
 * is str an VLAN ID <1-4094> ?
 */
int
is_vlan_id(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_VLAN_ID,
			 "^(([0]*[1-9])|([0]*[1-9][0-9]{1,2})|"
			 "([0]*[1-3][0-9]{1,3})|"
			 "([0]*40[0-8][0-9])|"
			 "([0]*409[0-4]))$");

	return (res == 1);
}

/*
 * is str a MAC address ?
 * 12 continual %x chars, or separated each %02x with ":" or "-"
 */
int
is_mac_addr(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_MAC_ADDR,
			 "^(([A-Fa-f0-9]{10})|(([A-Fa-f0-9]{2}[:\\-]){5}))"
			 "([A-Fa-f0-9]{2})$");

	return (res == 1);
}

/*
 * is str a word ?
 */
int
is_word(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_WORD, "^([a-zA-Z]+)(\\w|-)*$");

	return (res == 1);
}

/*
 * arbitrary str ?
 */
int
is_words(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_WORDS, "^(\\w|\\W)+$");

	return (res == 1);
}

/*
 * is str a arbitary word parameter (without space) ?
 */
int
is_param(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_PARAM, "^[\\w|\\-\\.]+$");

	return (res == 1);
}

/*
 * is str a HTTP URL ?
 */
int
is_http_url(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_HTTP_URL,
			 "^[hH][tT][tT][pP]:\\/\\/"
			 "\\w[\\w\\-\\.]*\\w+"
			 "(:(([0-9]{1,4})|([0-5][0-9]{1,4})|(6[0-5][0-5][0-3][0-5])))?"
			 "(\\/[\\w\\.\\-\\?#%=+&]*)*\\/?$");

	return (res == 1);
}

/*
 * is str a HTTPS URL ?
 */
int
is_https_url(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_HTTPS_URL,
			 "^[hH][tT][tT][pP][sS]:\\/\\/"
			 "\\w[\\w\\-\\.]*\\w+"
			 "(:(([0-9]{1,4})|([0-5][0-9]{1,4})|(6[0-5][0-5][0-3][0-5])))?"
			 "(\\/[\\w\\.\\-\\?#%=+&]*)*\\/?$");

	return (res == 1);
}

/*
 * is str a FTP file URL ?
 */
int
is_ftp_url(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	/* support user:passwd@ inside FTP url */
	res = pcre_match(str, LEX_FTP_URL,
			 "^[fF][tT][pP]:\\/\\/"
			 "(\\w[\\w\\-\\.]*:[\\w\\-\\.]+@)?"
			 "\\w[\\w\\-\\.]*\\w+"
			 "(\\/[\\w\\.\\-]*)+$");

	return (res == 1);
}

/*
 * is str a SCP file URL ?
 */
int
is_scp_url(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	/* must have user@ inside SCP url */
	res = pcre_match(str, LEX_SCP_URL,
			 "^[sS][cC][pP]:\\/\\/"
			 "\\w[\\w\\-\\.]*@"
			 "\\w[\\w\\-\\.]*\\w+"
			 "(:(([0-9]{1,4})|([0-5][0-9]{1,4})|(6[0-5][0-5][0-3][0-5])))?"
			 "(\\/[\\w\\.\\-]*)+$");

	return (res == 1);
}

/*
 * is str a TFTP file URL ?
 */
int
is_tftp_url(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_TFTP_URL,
			 "^[tT][fF][tT][pP]:\\/\\/"
			 "\\w[\\w\\-\\.]*\\w+"
			 "(\\/[\\w\\.\\-]*)+$");

	return (res == 1);
}

/*
 * is str a file name ?
 */
int
is_file_name(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_FILE_NAME,
			 "^(\\w[\\w+\\-\\_\\.]*\\w)$");

	return (res == 1);
}

/*
 * is str a file path ?
 */
int
is_file_path(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_FILE_PATH,
			 "^[\\/]?(\\w[\\w+\\-\\_\\.]*\\/)*"
			 "(\\w[\\w+\\-\\_\\.]*\\w)$");

	return (res == 1);
}

/*
 * is str a admin user id ?
 */
int
is_uid(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_UID, "^(\\w[\\w\\.\\-]*\\w+)$");

	return (res == 1);
}

/*
 * is str a clear password ?
 */
int
is_clear_passwd(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_CLEAR_PASSWD, "^([\\x21-\\x7e]{1,20})$");

	return (res == 1);
}

/*
 * is str a md5 crypted password ?
 */
int
is_md5_cipher(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_MD5_CIPHER,
			 "^(\\$1\\$"
			 "[\\dA-Za-z\\.\\/]{8}\\$"
			 "[\\dA-Za-z\\.\\/]{22})$");

	return (res == 1);
}

/*
 * is str a user@domain id ?
 */
int
is_net_uid(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_NET_UID,
			 "^(\\w[\\w\\.\\-]*\\w+)"
			 "(@\\w(\\w*\\.)+\\w+)$");

	return (res == 1);
}

/*
 * is str a user@ip6addr id ?
 */
int
is_net6_uid(char *str)
{
	char	name[128], *at, *addr;
	int	len;

	if (!str || !str[0]) return 0;

	if ((at = strchr(str, '@')) == NULL) return 0;
	if ((len = at - str) == 0) return 0;
	if (!isxdigit(*(at + 1)) && *(at + 1) != ':') return 0;

	addr = at + 1;

	bzero(name, sizeof(name));
	strncpy(name, str, len);

	return (is_uid(name) && is_ip6_addr(addr));
}

/*
 * is str a radius user id ?
 */
int
is_radius_uid(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	/* allow for PREFIX/user@domain roaming uid pattern */
	res = pcre_match(str, LEX_RADIUS_UID,
			 "^(\\w+\\/)?(\\w[\\w\\.\\-]*\\w+)"
			 "(@\\w[\\w\\-]*(\\.\\w[\\w\\-]*)*)?$");

	return (res == 1);
}

/*
 * is str a YYYYMMDDhhmm[ss] date time ?
 */
int
is_date_time(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	/* XXX we start up from 2015, just forget the past ... */
	res = pcre_match(str, LEX_DATE_TIME,
			 "^(20((1[5-9])|([2-9][0-9]))"
			 "((0[1-9])|(1[0-2]))"
			 "((0[1-9])|([1-2][0-9])|3[0-1])"
			 "(([0-1][0-9])|(2[0-3]))"
			 "([0-5][0-9]))"
			 "(\\.[0-5][0-9])?$");

	return (res == 1);
}

/*
 * is str a YYYYMMDD date ?
 */
int
is_date(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_DATE,
			 "^(20((1[5-9])|([2-9][0-9]))"
			 "((0[1-9])|(1[0-2]))"
			 "((0[1-9])|([1-2][0-9])|3[0-1]))$");

	return (res == 1);
}

/*
 * is str a Mbits bandwidth ?
 */
int
is_mbits_bw(char *str)
{
	int	res;

	if (!str || !str[0]) return 0;

	res = pcre_match(str, LEX_MBITS_BW, "^((\\d+)(\\.\\d*)?)M?$");

	return (res == 1);
}

#define	ETH_PREFIX	"eth"
#define	DEF_ETH_IFNUM	4
#define	MAX_ETH_IFNUM	10

#define	TUN_PREFIX	"tun"
#define	MAX_TUN_IFNUM	4

#define	PPP_PREFIX	"ppp"
#define	MAX_PPP_IFNUM	5

static int max_eth_ifindex = -1;

int
get_eth_ifnum()
{
#ifdef DEBUG_LEX_MAIN
	return 10;
#else
	FILE	*fp;
	char	line[256];
	char	*tok;
	int	n, ifidx = -1;

	fp = fopen("/proc/net/dev", "r");
	if (fp == NULL) {
		fprintf(stderr, "Open system dev file error\n");
		return DEF_ETH_IFNUM;
	}

	while (fgets(line, sizeof(line), fp)) {
		tok = strtok(line, " \t:");
		if (tok == NULL) continue;
		if (strncmp(tok, ETH_PREFIX, 3) != 0) continue;

		n = atoi(tok+3);
		if (n > ifidx)
			ifidx = n;
	}
	fclose(fp);

	if (ifidx < 0) {
		fprintf(stderr, "No ethernet dev found\n");
		return DEF_ETH_IFNUM;
	}

	++ifidx;
	return ((ifidx <= MAX_ETH_IFNUM) ? ifidx : MAX_ETH_IFNUM);
#endif
}

/*
 * get max ifindex by interface name prefix: 'eth', 'ppp' or 'tun'
 */
int
get_max_ifindex(char *prefix)
{
	if (!prefix || !prefix[0]) return -1;

	if (strcmp(prefix, ETH_PREFIX) == 0)
		return max_eth_ifindex;
	else if (strcmp(prefix, TUN_PREFIX) == 0)
		return (MAX_TUN_IFNUM - 1);
	else if (strcmp(prefix, PPP_PREFIX) == 0)
		return (MAX_PPP_IFNUM - 1);
	else
		return -1;
}

/*
 * is str an ethernet interface name ?
 */
int
is_eth_ifname(char *str)
{
	int	res;
	char	pattern[64];

	if (!str || !str[0]) return 0;

	if (max_eth_ifindex < 0)
		max_eth_ifindex = get_eth_ifnum() - 1;

	bzero(pattern, 64);
	snprintf(pattern, 64, "^%s[0-%d]$", ETH_PREFIX, max_eth_ifindex);

	res = pcre_match(str, LEX_ETH_IFNAME, pattern);

	return (res == 1);
}

/*
 * is str a tunnel interface name ?
 */
int
is_tun_ifname(char *str)
{
	int	res;
	char	pattern[64];

	if (!str || !str[0]) return 0;

	bzero(pattern, 64);
	snprintf(pattern, 64, "^%s[0-%d]$", TUN_PREFIX, MAX_TUN_IFNUM - 1);

	res = pcre_match(str, LEX_TUN_IFNAME, pattern);

	return (res == 1);
}

/*
 * is str a ppp interface name ?
 */
int
is_ppp_ifname(char *str)
{
	int	res;
	char	pattern[64];

	if (!str || !str[0]) return 0;

	bzero(pattern, 64);
	snprintf(pattern, 64, "^%s[0-%d]$", PPP_PREFIX, MAX_PPP_IFNUM - 1);

	res = pcre_match(str, LEX_PPP_IFNAME, pattern);

	return (res == 1);
}

/*
 * is str a empty line ?
 */
int
is_empty_line(char *str)
{
	if (!str || !str[0]) return 1;

	while (*str) {
		if (!isspace(*str++))
			return 0;
	}
	return 1;
}

/*
 * register a lex parsing entry
 */
static int
set_lex_ent(int type, char *name, lex_fun_t fun, char *help, char *prefix)
{
	if (type < 0 || type >= MAX_LEX_TYPE) {
		fprintf(stderr, "invalid lex index %d\n", type);
		return -1;
	}

	if (!name || !name[0]) {
		fprintf(stderr, "invalid lex name\n");
		return -1;
	}

	if (!help || !help[0]) {
		fprintf(stderr, "invalid lex help info\n");
		return -1;
	}

	bzero(&lex_ent[type], sizeof(struct lex_ent));
	strncpy(lex_ent[type].name, name, LEX_NAME_LEN-1);
	lex_ent[type].fun = fun;
	strncpy(lex_ent[type].help, help, LEX_TEXT_LEN-1);

	if (prefix && prefix[0])
		strncpy(lex_ent[type].prefix, prefix, LEX_TEXT_LEN-1);

	return 0;
}

/*
 * get lex_ent by type
 */
struct lex_ent *
get_lex_ent(int type)
{
	if (IS_VALID_LEX_TYPE(type))
		return (&lex_ent[type]);
	else
		return NULL;
}

/*
 * translate bit mask into ia mask
 */
in_addr_t
bits_to_netmask(int bits)
{
        if (bits <= 0)
		return 0;
	if (bits >= 32)
		return (in_addr_t)0xffffffff;
	else
		return (htonl(((in_addr_t)0xffffffff) << (32 - bits)));
}

/*
 * translate into ia mask into bits
 */
int
netmask_to_bits(in_addr_t s_addr)
{
	int	zero_bits = 0;

	s_addr = ntohl(s_addr);
	if (s_addr == 0)
		return 0;
	else if (s_addr == 0xffffffff)
		return 32;

	while ((s_addr & 0x01) != 0x01) { 
		s_addr >>= 1;
		zero_bits++;
	}
	return (32 - zero_bits);
}

/*
 * translate ipv4 blocks into subnet and mask
 */
int
get_subnet_mask(char *str, struct in_addr *ia_net, struct in_addr *ia_mask, int fix_net)
{
	char	buf[64];
	char	*slash, *net, *mask;
	int	bits;

	if (!str || !str[0] || !ia_net || !ia_mask) return 0;
	bzero(buf, 64);
	strncpy(buf, str, 63);

	if (is_ip_block(buf)) {
		net = buf;
		if ((slash = strrchr(buf, '/')) != NULL) {
			mask = slash + 1;
			*slash = '\0';
		} else
			mask = NULL;

		ia_net->s_addr = inet_addr(net);
		if (ia_net->s_addr == 0) {
			ia_mask->s_addr = 0;
			return 1;
		}

		if (mask == NULL) 
			bits = 32;
		else
			bits = atoi(mask);


		ia_mask->s_addr = bits_to_netmask(bits);
		if (fix_net) ia_net->s_addr &= ia_mask->s_addr;

		return 1;
	}

	return 0;
}

/*
 * xdigit => xval
 */
static u_char
get_xval(char ch)
{
	if (ch >= '0' && ch <='9')
		return (ch - '0');
	else if (ch >= 'A' && ch <= 'F')
		return (ch - 'A' + 10);
	else if (ch >= 'a' && ch <= 'f')
		return (ch - 'a' + 10);
	else
		return 0xff;
}

/*
 * get binary MAC address
 */
int
get_binary_mac(char *str, u_char *mac, int len)
{
	int	i, n;
	char	*ch;
	u_char	byte, val, hi = 0, lo = 0;

	if (!str || !str[0] || !mac || len < 6) return 0;
	if (!is_mac_addr(str)) return 0;
	bzero(mac, len);

	i = 0; n = 0;
	ch = str;
	while (n != 12 && *ch != '\0') {
		if ((val = get_xval(*ch)) == 0xff) {
			ch++;
			continue;
		}
		if ((n & 0x01) == 0x01) {
			lo = val;
			byte = (hi << 4) + lo;
			mac[i++] = byte;
			n++;
			ch++;
		} else {
			byte = 0;
			hi = val;
			n++;
			ch++;
		}
	}
	return 1;
}

/*
 * get formal xx:xx:xx:xx:xx:xx MAC address string
 */
int
get_formal_mac(char *str, char *mac_addr, int len)
{
	u_char raw_mac[6];

	bzero(mac_addr, len);

	if (get_binary_mac(str, &raw_mac[0], 6)) {
		snprintf(mac_addr, len,
			"%02x:%02x:%02x:%02x:%02x:%02x",
			raw_mac[0], raw_mac[1], raw_mac[2],
			raw_mac[3], raw_mac[4], raw_mac[5]);
		return 1;
	} else
		return 0;
}

/*
 * translate ip range
 */
int
get_ip_range(char *str, struct in_addr *ia_from, struct in_addr *ia_to)
{
	char	*hiphen, *from, *to;
	char	buf[80];
	in_addr_t s_tmp;

	if (!str || !str[0] || !ia_from || !ia_to) return 0;
	bzero(buf, 80);
	strncpy(buf, str, 79);

	if (!is_ip_range(buf)) return 0;

	from = buf;
	if ((hiphen = strrchr(buf, '-')) != NULL) {
		to = hiphen + 1;
		*hiphen = '\0';
	} else {
		to = NULL;
	}

	ia_from->s_addr = inet_addr(from);
	if (to != NULL) {
		ia_to->s_addr = inet_addr(to);
		if (ntohl(ia_to->s_addr) < ntohl(ia_from->s_addr)) {
			s_tmp = ia_from->s_addr;
			ia_from->s_addr = ia_to->s_addr;
			ia_to->s_addr = s_tmp;
		}
	} else {
		ia_to->s_addr = ia_from->s_addr;
	}

	return 1;
}

/*
 * translate layer4 port range
 */
int
get_port_range(char *str, u_short *port_from, u_short *port_to)
{
	char	*hiphen, *from, *to;
	char	buf[20];
	u_short tmp;

	if (!str || !str[0] || !port_from || !port_to) return 0;
	bzero(buf, 20);
	strncpy(buf, str, 19);

	if (!is_port_range(buf)) return 0;

	from = buf;
	if ((hiphen = strrchr(buf, '-')) != NULL) {
		to = hiphen + 1;
		*hiphen = '\0';
	} else {
		to = NULL;
	}

	*port_from = atoi(from);
	if (to != NULL) {
		*port_to = atoi(to);
		if (*port_to < *port_from) {
			tmp = *port_from;
			*port_from = *port_to;
			*port_to = tmp;
		}
	} else {
		*port_to = *port_from;
	}

	return 1;
}

/*
 * get elements from a http/https/ftp/tftp URI
 * call this func after URI lexically verified true
 */
int
get_uri_elements(char *str, char **pproto, char **phost,
		 char **ppath, char **pfile)
{
	char	*ptr;

	*pproto = *phost = *ppath = *pfile = NULL;

	if (strncasecmp(str, "http://", 7) == 0) {
		*(str + 4) = '\0';
		*phost = str + 7;
	} else if (strncasecmp(str, "https://", 8) == 0) {
		*(str + 5) = '\0';
		*phost = str + 8;
	} else if (strncasecmp(str, "scp://", 6) == 0) {
		*(str + 3) = '\0';
		*phost = str + 6;
	} else if (strncasecmp(str, "ftp://", 6) == 0) {
		*(str + 3) = '\0';
		*phost = str + 6;
		if (!(**phost)) return 0;
		if ((ptr = strchr(*phost, '@')) != NULL) {
			if (*(ptr + 1))
				*phost = ptr + 1;
			else
				return 0;
		}
	} else if (strncasecmp(str, "tftp://", 7) == 0) {
		*(str + 4) = '\0';
		*phost = str + 7;
	} else
		return 0;

	if (!(**phost)) return 0;
	*pproto = str;
	ptr = *phost;

	while (*ptr && *ptr != '/') ptr++;
	if (*ptr == '/') {
		*ptr = '\0';
		if (*(ptr + 1))
			*ppath = ptr + 1;
		else
			return 1;
	} else if (!(*ptr))
		return 1;

	if ((ptr = strrchr(*ppath, '/')) != NULL) {
		if (*(ptr + 1))
			*pfile = ptr + 1;
	} else
		*pfile = *ppath;

	return 1;
}

/*
 * get lex_ent by name
 */
struct lex_ent *
get_lex_ent_by_name(char *name)
{
	int	type;

	for (type = 0; type <= MAX_LEX_TYPE; type++) {
		if (lex_ent[type].name[0] &&
		    strcmp(lex_ent[type].name, name) == 0) {
			return (&lex_ent[type]);
		}
	}
	return NULL;
}

/*
 * init module
 */
int
lex_init(void)
{
	char	help_txt[LEX_TEXT_LEN];

	if (lex_init_ok) return 0;

	/* init pcre precompile memory */
	bzero(&pcre_cache[0], sizeof(pcre_cache));

	/* init lexicial paring entries */
	bzero(&lex_ent[0], sizeof(struct lex_ent) * MAX_LEX_TYPE);

	set_lex_ent(LEX_IP_ADDR, "IP_ADDR", is_ip_addr, "a.b.c.d", NULL);
	set_lex_ent(LEX_IP_MASK, "IP_MASK", is_ip_mask, "m.m.m.m", NULL);
	set_lex_ent(LEX_IP_PREFIX, "IP_PREFIX", is_ip_prefix, "a.b.c.d/<0~32>", NULL);
	set_lex_ent(LEX_IP_BLOCK, "IP_BLOCK", is_ip_block, "a.b.c.d[/<0~32>]", NULL);
	set_lex_ent(LEX_IP_RANGE, "IP_RANGE", is_ip_range, "a.b.c.d[-a.b.c.d]", NULL);
	set_lex_ent(LEX_IP6_ADDR, "IP6_ADDR", is_ip6_addr, "IP6Addr", NULL);
	set_lex_ent(LEX_IP4_IP6_ADDR, "IP4_IP6_ADDR", is_ip4_ip6_addr, "IP4Addr|IP6Addr", NULL);
	set_lex_ent(LEX_IP6_PREFIX, "IP6_PREFIX", is_ip6_prefix, "IP6Addr/Pfxlen", NULL);
	set_lex_ent(LEX_IP6_BLOCK, "IP6_BLOCK", is_ip6_block, "IP6Addr[/Pfxlen]", NULL);
	set_lex_ent(LEX_IFNAME_ADDR, "IFNAME_ADDR", is_ifname_addr, "Interface name|addr", NULL);
	set_lex_ent(LEX_PORT, "PORT", is_port, "<0~65535>", NULL);
	set_lex_ent(LEX_PORT_RANGE, "PORT_RANGE", is_port_range, "<0~65535>[-<0~65535>]", NULL);
	set_lex_ent(LEX_VLAN_ID, "VLAN_ID", is_vlan_id, "<1-4094>", NULL);
	set_lex_ent(LEX_MAC_ADDR, "MAC_ADDR", is_mac_addr, "xx:xx:xx:xx:xx:xx", NULL);
	set_lex_ent(LEX_WORD, "WORD", is_word, "Word", NULL);
	set_lex_ent(LEX_WORDS, "WORDS", is_words, "\"Words...\"", NULL);
	set_lex_ent(LEX_PARAM, "PARAM", is_param, "Parameter", NULL);
	set_lex_ent(LEX_INT, "INT", is_int, "Integer", NULL);
	set_lex_ent(LEX_DECIMAL, "DECIMAL", is_decimal, "Decimal", NULL);
	set_lex_ent(LEX_HOST_NAME, "HOST_NAME", is_host_name, "Host", NULL);
	set_lex_ent(LEX_HOST, "HOST", is_host, "Host|a.b.c.d", NULL);
	set_lex_ent(LEX_HOST6, "HOST6", is_host6, "Host|IP4|IP6", NULL);
	set_lex_ent(LEX_DOMAIN_NAME, "DOMAIN_NAME", is_domain_name, "Domain", NULL);
	set_lex_ent(LEX_DOMAIN_WILDCARD, "DOMAIN_WILDCARD", is_domain_wildcard, "*.domain", NULL);
	set_lex_ent(LEX_DOMAIN_EXT, "DOMAIN_EXT", is_domain_ext, "[*.]domain", NULL);
	set_lex_ent(LEX_EMAIL_ADDR, "EMAIL_ADDR", is_email_addr, "user@domain.name", NULL);
	set_lex_ent(LEX_HTTP_URL, "HTTP_URL", is_http_url, "http://host/path", NULL);
	set_lex_ent(LEX_HTTPS_URL, "HTTPS_URL", is_https_url, "https://host/path", NULL);
	set_lex_ent(LEX_FTP_URL, "FTP_URL", is_ftp_url, "ftp://[user:password@]host/path", NULL);
	set_lex_ent(LEX_SCP_URL, "SCP_URL", is_scp_url, "scp://user@host/path", NULL);
	set_lex_ent(LEX_TFTP_URL, "TFTP_URL", is_tftp_url, "tftp://host/path", NULL);
	set_lex_ent(LEX_FILE_NAME, "FILE", is_file_name, "File", NULL);
	set_lex_ent(LEX_FILE_PATH, "PATH", is_file_path, "Path", NULL);
	set_lex_ent(LEX_UID, "UID", is_uid, "UserID", NULL);
	set_lex_ent(LEX_NET_UID, "NET_UID", is_net_uid, "user@host", NULL);
	set_lex_ent(LEX_NET6_UID, "NET6_UID", is_net6_uid, "user@IP6Addr", NULL);
	set_lex_ent(LEX_RADIUS_UID, "RADIUS_UID", is_radius_uid, "[prefix/]uid[@domain]", NULL);
	set_lex_ent(LEX_CLEAR_PASSWD, "CLEAR_PASSWD", is_clear_passwd, "ClearPassword", NULL);
	set_lex_ent(LEX_MD5_CIPHER, "MD5_CIPHER", is_md5_cipher, "MD5Cipher", NULL);
	set_lex_ent(LEX_DATE_TIME, "DATE_TIME", is_date_time, "YYYYMMDDhhmm[.ss]", NULL);
	set_lex_ent(LEX_DATE, "DATE", is_date, "YYYYMMDD", NULL);
	set_lex_ent(LEX_MBITS_BW, "BW", is_mbits_bw, "<Decimal>[M]", NULL);

	if (max_eth_ifindex < 0)
		max_eth_ifindex = get_eth_ifnum() - 1;

	sprintf(help_txt, "%s[0~%d]", ETH_PREFIX, max_eth_ifindex);
	set_lex_ent(LEX_ETH_IFNAME, "ETH_IFNAME", is_eth_ifname, help_txt, ETH_PREFIX);

	if (max_eth_ifindex > 6)
		sprintf(help_txt, "%s[1~4,6~%d]", ETH_PREFIX, max_eth_ifindex);
	else
		sprintf(help_txt, "%s[1~%d]", ETH_PREFIX,
			(max_eth_ifindex > 4) ? 4:max_eth_ifindex);

	sprintf(help_txt, "%s[0~%d]", TUN_PREFIX, MAX_TUN_IFNUM - 1);
	set_lex_ent(LEX_TUN_IFNAME, "TUN_IFNAME", is_tun_ifname, help_txt, TUN_PREFIX);

	sprintf(help_txt, "%s[0~%d]", PPP_PREFIX, MAX_PPP_IFNUM - 1);
	set_lex_ent(LEX_PPP_IFNAME, "PPP_IFNAME", is_ppp_ifname, help_txt, PPP_PREFIX);

	lex_init_ok = 1;
	return 0;
}

/*
 * exit module
 */
void
lex_exit(void)
{
	int	i;

	/* free all pcre precompile cache memory */
	for (i = 0; i < MAX_LEX_TYPE; i++) {
		if (pcre_cache[i] != NULL) {
			pcre_free(pcre_cache[i]);
			pcre_cache[i] = NULL;
		}
	}
	lex_init_ok = 0;
}

#ifdef DEBUG_LEX_MAIN
int
main(int argc, char **argv)
{
	int	i, j, k, res;
	struct in_addr ia_net, ia_mask;
	struct in_addr ia_from, ia_to;
	u_short	port_from, port_to;
	u_char	mac[6];

	lex_init();

	for (i = 1; i < argc; i++) {
		for (j = 1; j < MAX_LEX_TYPE; j++) {
			if (lex_ent[j].name[0] && lex_ent[j].fun) {
				res = lex_ent[j].fun(argv[i]);
				if (res != 1) continue;

				printf("%s(\"%s\") = true, "
					"help = %s,  pfx = %s\n"
					lex_ent[j].name, argv[i],
					lex_ent[j].help,
					(lex_ent[j].prefix[0]) ?
						lex_ent[j].prefix:"NULL");

				switch (j) {
				case LEX_IP_PREFIX:
					get_subnet_mask(argv[i], &ia_net, &ia_mask, 0);
					printf("	addr: %s\n", inet_ntoa(ia_net));
					printf("	mask: %s\n", inet_ntoa(ia_mask));
					break;
				case LEX_IP_BLOCK:
					get_subnet_mask(argv[i], &ia_net, &ia_mask, 1);
					printf("	subnet: %s\n", inet_ntoa(ia_net));
					printf("	mask: %s\n", inet_ntoa(ia_mask));
					break;
				case LEX_IP_RANGE:
					get_ip_range(argv[i], &ia_from, &ia_to);
					printf("	from: %s\n", inet_ntoa(ia_from));
					printf("	to: %s\n", inet_ntoa(ia_to));
					break;
				case LEX_PORT_RANGE:
					get_port_range(argv[i], &port_from, &port_to);
					printf("	port range: %d-%d\n", port_from, port_to);
					break;
				case LEX_MAC_ADDR:
					get_binary_mac(argv[i], mac, 6);
					printf("	mac: ");
					for (k = 0; k < 6; k++) printf("%02X", mac[k]);
					printf("\n");
				default:
					break;
				}
			}
		}
	}

	lex_exit();
	return 0;
}
#endif	/* DEBUG_LEX_MAIN */
