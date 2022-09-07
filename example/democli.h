/*
 *  democli, a simple program to demonstrate how to use libocli
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

#ifndef _DEMOCLI_H
#define _DEMOCLI_H

#include <ocli/ocli.h>

#define INTERFACE_VIEW	0x08

/* customized lex type */
#define	LEX_IFINDEX	LEX_CUSTOM_TYPE(0)	
#define	LEX_ETH_IFNAME	LEX_CUSTOM_TYPE(1)	

extern int mylex_init();

/* interface of sys.c module */
extern void democli_set_view(int view);
extern int cmd_sys_init();

/* interface of other modules */
extern int cmd_net_utils_init();
extern int cmd_route_init();
extern int cmd_show_init();

extern char *get_current_ifname();
extern int cmd_interface_init();

#endif	/* _DEMOCLI_H */
