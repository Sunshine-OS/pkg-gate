/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	from OpenSolaris "libadm.h	1.10	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)libadm.h	1.5 (gritter) 2/25/07
 */

#ifndef	__PKG_LIBADM_H__
#define	__PKG_LIBADM_H__

#include <sys/types.h>
#include <sys/dklabel.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <stdio.h>
#include <pkgstrct.h>
#include <pkginfo.h>
#include <valtools.h>
#include "install.h"

#undef	__P
#if defined(__STDC__)
#define	__P(protos) protos
#else	/* __STDC__ */
#define	__P(protos) ()
#endif	/* __STDC__ */

extern int ckquit;
extern int ckwidth;
extern int ckindent;

/*
 * ckdate.c
 */
extern int	ckdate_err __P((char *fmt, char *error));
extern int	ckdate_hlp __P((char *fmt, char *help));
extern int	ckdate_val __P((char *fmt, char *input));
extern int	ckdate __P((char *date, char *fmt, char *defstr, char *error,
				char *help, char *prompt));

/*
 * ckgid.c
 */
extern int	ckgid_dsp __P((void));
extern int	ckgid_val __P((char *grpnm));
extern int	ckgrpfile __P((void));
extern void	ckgid_err __P((int disp, char *error));
extern void	ckgid_hlp __P((int disp, char *help));
extern int	ckgid __P((char *gid, short disp, char *defstr, char *error,
				char *help, char *prompt));

/*
 * ckint.c
 */
extern int	ckint_val __P((char *value, short base));
extern void	ckint_err __P((short base, char *error));
extern void	ckint_hlp __P((short base, char *help));
extern int	ckint __P((long *intval, short base, char *defstr, char *error,
				char *help, char *prompt));

/*
 * ckitem.c
 */
extern CKMENU	*allocmenu __P((char *label, int attr));
extern void	ckitem_err __P((CKMENU *menup, char *error));
extern void	ckitem_hlp __P((CKMENU *menup, char *help));
extern int	ckitem __P((CKMENU *menup, char *item[], short max,
				char *defstr, char *error, char *help,
				char *prompt));
extern int	setitem __P((CKMENU *menup, char *choice));
extern int	setinvis __P((CKMENU *menup, char *choice));
extern void	printmenu __P((CKMENU *menup));

/*
 * ckkeywd.c
 */
extern int	ckkeywd __P((char *strval, char *keyword[], char *defstr,
				char *error, char *help, char *prompt));

/*
 * ckpath.c
 */
extern int	ckpath_stx __P((int pflags));
extern int	ckpath_val __P((char *path, int pflags));
extern void	ckpath_err __P((int pflags, char *error, char *input));
extern void	ckpath_hlp __P((int pflags, char *help));
extern int	ckpath __P((char *pathval, int pflags, char *defstr,
				char *error, char *help, char *prompt));

/*
 * ckrange.c
 */
extern void	ckrange_err __P((long lower, long upper, int base,
				char *error));
extern void	ckrange_hlp __P((long lower, long upper, int base, char *help));
extern int	ckrange_val __P((long lower, long upper, int base,
				char *input));
extern int	ckrange __P((long *rngval, long lower, long upper, short base,
				char *defstr, char *error, char *help,
				char *prompt));

/*
 * ckstr.c
 */
extern int	ckstr_val __P((char *regexp[], int length, char *input));
extern void	ckstr_err __P((char *regexp[], int length, char *error,
				char *input));
extern void	ckstr_hlp __P((char *regexp[], int length, char *help));
extern int	ckstr __P((char *strval, char *regexp[], int length,
				char *defstr, char *error, char *help,
				char *prompt));

/*
 * cktime.c
 */
extern int	cktime_val __P((char *fmt, char *input));
extern int	cktime_err __P((char *fmt, char *error));
extern int	cktime_hlp __P((char *fmt, char *help));
extern int	cktime_fmtcheck __P((char *fmt));
extern int	cktime __P((char *tod, char *fmt, char *defstr, char *error,
				char *help, char *prompt));

/*
 * ckuid.c
 */
extern int	ckuid_dsp __P((void));
extern int	ckuid_val __P((char *usrnm));
extern int	ckpwdfile __P((void));
extern void	ckuid_err __P((short disp, char *error));
extern void	ckuid_hlp __P((int disp, char *help));
extern int	ckuid __P((char *uid, short disp, char *defstr, char *error,
				char *help, char *prompt));

/*
 * ckyorn.c
 */
extern int	ckyorn_val __P((char *str));
extern void	ckyorn_err __P((char *error));
extern void	ckyorn_hlp __P((char *help));
extern int	ckyorn __P((char *yorn, char *defstr, char *error, char *help,
				char *prompt));

/*
 * fulldevnm.c
 */
extern char	*getfullblkname __P((char *cp));
extern char	*getfullrawname __P((char *cp));

/*
 * getinput.c
 */
extern int	getinput __P((char *s));

/*
 * memory.c
 */
extern long	sysmem __P((void));
extern long	asysmem __P((void));

/*
 * pkginfo.c
 */
extern int	pkginfo __P((struct pkginfo *info, char *pkginst, ...));
extern int	fpkginfo __P((struct pkginfo *info, char *pkginst));
extern char	*fpkginst __P((char *pkg, ...));

/*
 * pkgnmchk.c
 */
extern int	pkgnmchk __P((register char *pkg, register char *spec,
				int presvr4flg));

/*
 * pkgparam.c
 */
extern int	pkginfofind __P((char *, char *, char *));
extern FILE	*pkginfopen __P((char *, char *));
extern char	*fpkgparam __P((FILE *fp, char *param));
extern char	*pkgparam __P((char *pkg, char *param));
extern void	set_PKGpaths __P((char *path));
extern char	*get_PKGLOC __P((void));
extern char	*get_PKGADM __P((void));
extern void	set_PKGADM(char *newpath);
extern void	set_PKGLOC(char *newpath);
extern void	set_install_root(char *);
extern char	*get_install_root(void);

/*
 * puterror.c
 */
extern void	puterror __P((FILE *fp, char *defmesg, char *error));

/*
 * puthelp.c
 */
extern void	puthelp __P((FILE *fp, char *defmesg, char *help));

/*
 * putprmpt.c
 */
extern void	putprmpt __P((FILE *fp, char *prompt, char *choices[],
				char *defstr));

/*
 * puttext.c
 */
extern int	puttext __P((FILE *fp, char *str, int lmarg, int rmarg));

/*
 * from OpenSolaris libc
 */
extern size_t	strlcat(char *, const char *, size_t);
extern size_t	strlcpy(char *, const char *, size_t);
extern void	closefrom(int);
extern int	sigsend(idtype_t, id_t, int);
extern int	cftime(char *, char *, const time_t *);
extern char	*getpassphrase(const char *);
extern int	mkdirp(const char *, mode_t);
extern int	ascftime(char *, const char *, const struct tm *);

#if defined(lint) && !defined(gettext)
#define	gettext(x)	x
#endif	/* defined(lint) && !defined(gettext) */

#endif	/* __PKG_LIBADM_H__ */
