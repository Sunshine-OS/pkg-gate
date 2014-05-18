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

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

/*	from OpenSolaris "isdir.c	1.11	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)isdir.c	1.3 (gritter) 2/24/07
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <archives.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "pkglocale.h"
#include "pkglibmsgs.h"

/*
 * Defines for cpio/compression checks.
 */
#define	BIT_MASK		0x1f
#define	BLOCK_MASK		0x80

#define	MASK_CK(x, y)	(((x) & (y)) == (y))
#define	ISCOMPCPIO	((unsigned char) cm.c_mag[0] == m_h[0] && \
			(unsigned char) cm.c_mag[1] == m_h[1] && \
			(MASK_CK((unsigned char) cm.c_mag[2], BLOCK_MASK) || \
			MASK_CK((unsigned char) cm.c_mag[2], BIT_MASK)))

#define	ISCPIO		(cm.b_mag != CMN_BIN && \
			(strcmp(cm.c_mag, CMS_ASC) == 0) && \
			(strcmp(cm.c_mag, CMS_CHR) == 0) && \
			(strcmp(cm.c_mag, CMS_CRC) == 0))

/* character array used to hold dfs types database contents */

static long		numRemoteFstypes = -1;
static char		**remoteFstypes = (char **)NULL;

/* forward declarations */

static void _InitRemoteFstypes(void);

int isFdRemote(int a_fd);
int isFstypeRemote(char *a_fstype);
int isdir(char *path);
int isfile(char *dir, char *file);
int iscpio(char *path, int *iscomp);

/*
 * Name:	isdir
 * Description:	determine if specified path exists and is a directory
 * Arguments:	path - pointer to string representing the path to verify
 * returns: 0 - directory exists
 *	    1 - directory does not exist or is not a directory
 * NOTE:	errno is set appropriately
 */

int
isdir(char *path)
{
	struct stat statbuf;

	/* return error if path does not exist */

	if (stat(path, &statbuf) != 0) {
		return (1);
	}

	/* return error if path is not a directory */

	if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
		errno = ENOTDIR;
		return (1);
	}

	return (0);
}

/*
 * Name:	isfile
 * Description:	determine if specified path exists and is a directory
 * Arguments:	dir - pointer to string representing the directory where
 *			the file is located
 *			== NULL - use "file" argument only
 *		file - pointer to string representing the file to verify
 * Returns:	0 - success - file exists
 *		1 - failure - file does not exist OR is not a file
 * NOTE:	errno is set appropriately
 */

int
isfile(char *dir, char *file)
{
	struct stat statbuf;
	char	path[PATH_MAX];

	/* construct full path if directory specified */

	if (dir) {
		(void) snprintf(path, sizeof (path), "%s/%s", dir, file);
		file = path;
	}

	/* return error if path does not exist */

	if (stat(file, &statbuf) != 0) {
		return (1);
	}

	/* return error if path is a directory */

	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
		errno = EISDIR;
		return (1);
	}

	/* return error if path is not a file */

	if ((statbuf.st_mode & S_IFMT) != S_IFREG) {
		errno = EINVAL;
		return (1);
	}

	return (0);
}

int
iscpio(char *path, int *iscomp)
{
	/*
	 * Compressed File Header.
	 */
	unsigned char m_h[] = { "\037\235" };		/* 1F 9D */

	static union {
		short int	b_mag;
		char		c_mag[CMS_LEN];
	}	cm;

	struct stat	statb;
	int		fd;


	*iscomp = 0;

	if ((fd = open(path, O_RDONLY, 0)) == -1) {
		if (errno != ENOENT) {
			perror("");
			(void) fprintf(stderr, pkg_gt(ERR_ISCPIO_OPEN), path);
		}
		return (0);
	} else {
		if (fstat(fd, &statb) == -1) {
			perror("");
			(void) fprintf(stderr, pkg_gt(ERR_ISCPIO_FSTAT), path);
			(void) close(fd);
			return (0);
		} else {
			if (S_ISREG(statb.st_mode)) {	/* Must be a file */
				if (read(fd, cm.c_mag, sizeof (cm.c_mag)) !=
				    sizeof (cm.c_mag)) {
					perror("");
					(void) fprintf(stderr,
					    pkg_gt(ERR_ISCPIO_READ), path);
					(void) close(fd);
					return (0);
				}
				/*
				 * Try to determine if the file is a compressed
				 * file, if that fails, try to determine if it
				 * is a cpio archive, if that fails, then we
				 * fail!
				 */
				if (ISCOMPCPIO) {
					*iscomp = 1;
					(void) close(fd);
					return (1);
				} else if (ISCPIO) {
					(void) fprintf(stderr,
					    pkg_gt(ERR_ISCPIO_NOCPIO),
					    path);
					(void) close(fd);
					return (0);
				}
				(void) close(fd);
				return (1);
			} else {
				(void) close(fd);
				return (0);
			}
		}
	}
}

/*
 * Name:	isFdRemote
 * Description:	determine if an open file is local or remote
 * Arguments:	a_fd - [RO, *RO] - (int)
 *			Integer representing open file to check
 * Returns:	int
 *			1 - the path is remote
 *			0 - the path is local to this system
 *			-1 - cannot determine if path is remote or local
 */

int
isFdRemote(int a_fd)
{
	int		r;
	struct stat	statbuf;

	r = fstat(a_fd, &statbuf);
	if (r < 0) {
		return (-1);
	}

#ifdef	__sun
	return (isFstypeRemote(statbuf.st_fstype));
#else
	return (-1);
#endif
}

/*
 * Name:	isFstypeRemote
 * Description:	determine if a file system type is remote (distributed)
 * Arguments:	a_fstype - [RO, *RO] - (char *)
 *			Pointer to string representing the file system type
 *			to check
 * Returns:	int
 *			1 - the file system type is remote
 *			0 - the file system type is local to this system
 */

int
isFstypeRemote(char *a_fstype)
{
	int	i;

	/* initialize the list if it is not yet initialized */

	if (numRemoteFstypes == -1) {
		_InitRemoteFstypes();
	}

	/* scan the list looking for the specified type */

	for (i = 0; i < numRemoteFstypes; i++) {
		if (strcmp(remoteFstypes[i], a_fstype) == 0) {
			return (1);
		}
	}

	/* type not found in remote file system type list - is not remote */

	return (0);
}

/*
 * Name:	_InitRemoteFstypes
 * Description:	initialize table of remote file system type names
 * Arguments:	none
 * Returns:	none
 * Side Effects:
 *	- The global array "(char **)remoteFstypes" is set to the
 *	  address of an array of string pointers, each of which represents
 *	  a single remote file system type
 *	- The global variable "(long) numRemoteFstypes" is set to the total
 *	  number of remote file system type strings (names) that are
 *	  contained in the "remoteFstypes" global array.
 *	- numRemoteFstypes is initialized to "-1" before any attempt has been
 *	  made to read the remote file system type name database.
 */
static void
_InitRemoteFstypes(void)
{
	/* start with zero */
	numRemoteFstypes = 0;

	/* shortcut - don't bother with fstypes(4) */
	remoteFstypes = (char **)realloc(remoteFstypes,
				sizeof (char *) * (numRemoteFstypes+3));
	remoteFstypes[numRemoteFstypes++] = "nfs";	/* +1 */
	remoteFstypes[numRemoteFstypes++] = "autofs";	/* +2 */
	remoteFstypes[numRemoteFstypes++] = "cachefs";	/* +3 */
}
