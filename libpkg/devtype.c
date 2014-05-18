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
 * Copyright 1996 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

/*	from OpenSolaris "devtype.c	1.9	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)devtype.c	1.3 (gritter) 2/25/07
 */

/*LINTLIBRARY*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pkgdev.h>
#include <pkglib.h>

int
devtype(char *alias, struct pkgdev *devp)
{
	char *name;
	devp->mntflg = 0;
	devp->name = alias;
	devp->dirname = devp->pathname = devp->mount = NULL;
	devp->fstyp = devp->cdevice = devp->bdevice = devp->norewind = NULL;
	devp->rdonly = 0;
	devp->capacity = 0;

	/* see if alias represents an existing file */
	if (alias[0] == '/') {
		if (!isdir(alias)) {
			devp->dirname = devp->name;
			return (0);
		}
	}

	if (!isdir(alias))
		devp->dirname = alias;
	else
		devp->pathname = alias;
	return (0);
}
