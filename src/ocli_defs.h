/*
 *  libocli, A general C library to provide a open-source cisco style
 *  command line interface.
 *
 *  Copyright (C) 2015 Digger Wu (digger.wu@linkbroad.com)
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

#ifndef	_OCLI_DEFS_H
#define _OCLI_DEFS_H

/*
 * the view definition, total 32bits, 1 bit for each view
 */
#define	ALL_VIEW_MASK		0xffff
#define	SUPER_VIEW		ALL_VIEW_MASK

#define	BASIC_VIEW		0x0001
#define	ENABLE_VIEW		0x0002
#define	CONFIG_VIEW		0x0004

#define	EXT_ENABLE_VIEW		(ENABLE_VIEW|BASIC_VIEW)
#define	EXT_CONFIG_VIEW		(CONFIG_VIEW|EXT_ENABLE_VIEW)

/* undo is only available above configure view */
#define	UNDO_VIEW_MASK		(SUPER_VIEW & ~(EXT_ENABLE_VIEW))

/*
 * undo command, default cisco "no COMMAND ..." style
 */
#ifndef	UNDO_CMD
#ifdef	HUAWEI_STYLE
#define	UNDO_CMD	"undo"
#else
#define	UNDO_CMD	"no"
#endif
#endif	/* UNDO_CMD */

/*
 * manual command, default Unix "man COMMAND" style
 */
#ifndef	MANUAL_CMD
#ifdef	GANGNAM_STYLE
#define	MANUAL_CMD	"help"
#else
#define	MANUAL_CMD	"man"
#endif
#endif	/* MANUAL_CMD */

#endif	/* _OCLI_DEFS_H */
