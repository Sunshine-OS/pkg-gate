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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef __PKG_PKGINSTALL_H__
#define	__PKG_PKGINSTALL_H__

/*	from OpenSolaris "pkginstall.h	1.13	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pkginstall.h	1.2 (gritter) 2/24/07
 */

#ifdef __cplusplus
extern "C" {
#endif

/* cppath() variables */
#define	DIR_DISPLAY	0x0001	/* display implied directories created */
#define	MODE_SRC	0x0002	/* set mode to mode of source file */
#define	MODE_SET	0x0004	/* set mode to mode passed in as argument */
#define	MODE_0666	0x0008	/* force mode to 0666 */

/* special stdin for request scripts */
#define	REQ_STDIN	"/dev/tty"

/* response file writability status */
#define	RESP_WR		0	/* Response file is writable. */
#define	RESP_RO		1	/* Read only. */

#ifdef __STDC__
#ifndef __P
#define	__P(x)	x
#endif
#else
#ifndef __P
#define	__P(x)	()
#endif
#endif /* __STDC__ */

extern int	cppath __P((int ctrl, char *f1, char *f2, mode_t mode));
extern void	backup __P((char *path, int mode));
extern void	pkgvolume __P((struct pkgdev *devp, char *pkg, int part,
		    int nparts));
extern void	quit __P((int exitval));
extern void	ckreturn __P((int retcode, char *msg));
extern int	sortmap __P((struct cfextra ***extlist, VFP_T *pkgmapVfp,
			VFP_T *mapvfp, VFP_T *tmpvfp, char *a_zoneName));
extern void merginfo __P((struct cl_attr **pclass, int install_from_pspool));
extern void	set_infoloc __P((char *real_pkgsav));
extern int	pkgenv __P((char *pkginst, char *p_pkginfo, char *p_pkgmap));
extern void	instvol __P((struct cfextra **extlist, char *srcinst, int part,
			int nparts, VFP_T **a_cfVfp, VFP_T **a_cfTmpVfp,
			char **r_updated, char **r_skipped,
			char *a_zoneName));
extern int	reqexec __P((int update, char *script,
			boolean_t enable_root_user));
extern int	chkexec __P((int update, char *script));
extern int	rdonly_respfile __P((void));
extern int	is_a_respfile __P((void));
extern char	*get_respfile __P((void));
extern int	set_respfile __P((char *respfile, char *pkginst,
		    int resp_stat));
extern void	predepend __P((char *oldpkg));
extern void	cksetPreinstallCheck __P((boolean_t a_preinstallCheck));
extern void	cksetZoneName __P((char *a_zoneName));
extern int	cksetuid __P((void));
extern int	ckconflct __P((void));
extern int	ckpkgdirs __P((void));
extern int	ckspace __P((void));
extern int	ckdepend __P((void));
extern int	ckrunlevel __P((void));
extern int	ckpartial __P((void));
extern int	ckpkgfiles __P((void));
extern int	ckpriv __P((void));
extern void	is_WOS_arch __P((void));
extern void	ckdirs __P((void));
extern char	*getinst __P((int *updatingExisting, struct pkginfo *info,
			int npkgs, boolean_t a_preinstallCheck));
extern int	is_samepkg __P((void));
extern int	dockspace __P((char *spacefile));

extern boolean_t	rm_all_pkg_entries(char *, char *);

#ifdef __cplusplus
}
#endif

#endif	/* __PKG_PKGINSTALL_H__ */
