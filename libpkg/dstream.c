/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

/*	from OpenSolaris "dstream.c	1.19	07/02/02 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)dstream.c	1.7 (gritter) 2/25/07
 */

/*LINTLIBRARY*/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
//#include <sys/sysmacros.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <openssl/err.h>
#include <pkglib.h>
#include <pkglibmsgs.h>
#include "pkglocale.h"
#include "libadm.h"

#ifdef __FreeBSD__ 
	#define O_LARGEFILE 0 
#endif 

/* libadm.a */
extern int	pkgnmchk(register char *pkg, register char *spec,
				int presvr4flg);

#define	CMDSIZ	512
#define	LSIZE	128
#define	DDPROC		BINDIR "/dd"
#define	CPIOPROC	BINDIR "/cpio"

struct dstoc {
	int	cnt;
	char	pkg[NON_ABI_NAMELNGTH];
	int	nparts;
	long	maxsiz;
	char    volnos[128];
	struct dstoc *next;
} *ds_head, *ds_toc;

#define	ds_nparts	ds_toc->nparts
#define	ds_maxsiz	ds_toc->maxsiz

int	ds_totread; 	/* total number of parts read */
int	ds_fd = -1;
int	ds_curpartcnt = -1;

int	ds_next(char *device, char *instdir);
int	ds_ginit(char *device);
int	ds_close(int pkgendflg);

static FILE	*ds_pp;
static int	ds_realfd = -1; 	/* file descriptor for real device */
static int	ds_read; 	/* number of parts read for current package */
static int	ds_volno; 	/* volume number of current volume */
static int	ds_volcnt; 	/* total number of volumes */
static char	ds_volnos[128]; 	/* parts/volume info */
static char	*ds_device;
static int	ds_volpart;	/* number of parts read in current volume, */
						/* including skipped parts */
static int	ds_bufsize;
static int	ds_skippart; 	/* number of parts skipped in current volume */

static int	ds_getnextvol(char *device);
static int	ds_skip(char *device, int nskip);

void
ds_order(char *list[])
{
	struct dstoc *toc_pt;
	register int j, n;
	char	*pt;

	toc_pt = ds_head;
	n = 0;
	while (toc_pt) {
		for (j = n; list[j]; j++) {
			if (strcmp(list[j], toc_pt->pkg) == 0) {
				/* just swap places in the array */
				pt = list[n];
				list[n++] = list[j];
				list[j] = pt;
			}
		}
		toc_pt = toc_pt->next;
	}
}

static char *pds_header;
static char *ds_header;
static char *ds_header_raw;
static int ds_headsize;

static char *
ds_gets(char *buf, int size)
{
	int length;
	char *nextp;

	nextp = strchr(pds_header, '\n');
	if (nextp == NULL) {
		length = strlen(pds_header);
		if (length > size)
			return (0);
		if ((ds_header = (char *)realloc(ds_header,
		    ds_headsize + BLK_SIZE)) == NULL)
			return (0);
		if (read(ds_fd, ds_header + ds_headsize, BLK_SIZE) < BLK_SIZE)
			return (0);
		ds_headsize += BLK_SIZE;
		nextp = strchr(pds_header, '\n');
		if (nextp == NULL)
			return (0);
		*nextp = '\0';
		if (length + (int)strlen(pds_header) > size)
			return (0);
		(void) strncpy(buf + length, pds_header, strlen(pds_header));
		buf[length + strlen(pds_header)] = '\0';
		pds_header = nextp + 1;
		return (buf);
	}
	*nextp = '\0';
	if ((int)strlen(pds_header) > size)
		return (0);
	(void) strncpy(buf, pds_header, strlen(pds_header));
	buf[strlen(pds_header)] = '\0';
	pds_header = nextp + 1;
	return (buf);
}

/*
 * function to determine if media is datastream or mounted
 * floppy
 */
int
ds_readbuf(char *device)
{
	char buf[BLK_SIZE];

	if (ds_fd >= 0)
		(void) close(ds_fd);
	if ((ds_fd = open(device, O_RDONLY | O_LARGEFILE)) >= 0 &&
	    read(ds_fd, buf, BLK_SIZE) == BLK_SIZE &&
	    strncmp(buf, HDR_PREFIX, 20) == 0) {
		if ((ds_header = (char *)calloc(BLK_SIZE, 1)) == NULL) {
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_MEM));
			(void) ds_close(0);
			return (0);
		}
		memcpy(ds_header, buf, BLK_SIZE);
		ds_headsize = BLK_SIZE;

		if (ds_ginit(device) < 0) {
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_OPEN), device, errno);
			(void) ds_close(0);
			return (0);
		}
		return (1);
	} else if (ds_fd >= 0) {
		(void) close(ds_fd);
		ds_fd = -1;
	}
	return (0);
}

/*
 * Determine how many additional volumes are needed for current package.
 * Note: a 0 will occur as first volume number when the package begins
 * on the next volume.
 */
static int
ds_volsum(struct dstoc *toc)
{
	int curpartcnt, volcnt;
	char volnos[128], tmpvol[128];
	if (toc->volnos[0]) {
		int index, sum;
		sscanf(toc->volnos, "%d %[ 0-9]", &curpartcnt, volnos);
		volcnt = 0;
		sum = curpartcnt;
		while (sum < toc->nparts && sscanf(volnos, "%d %[ 0-9]",
		    &index, tmpvol) >= 1) {
			(void) strcpy(volnos, tmpvol);
			volcnt++;
			sum += index;
		}
		/* side effect - set number of parts read on current volume */
		ds_volpart = index;
		return (volcnt);
	}
	ds_volpart += toc->nparts;
	return (0);
}

/* initialize ds_curpartcnt and ds_volnos */
static void
ds_pkginit(void)
{
	if (ds_toc->volnos[0])
		sscanf(ds_toc->volnos, "%d %[ 0-9]", &ds_curpartcnt, ds_volnos);
	else
		ds_curpartcnt = -1;
}

/*
 * functions to pass current package info to exec'ed program
 */
void
ds_putinfo(char *buf)
{
	(void) sprintf(buf, "%d %d %d %d %d %d %d %d %d %ld %s",
	    ds_fd, ds_realfd, ds_volcnt, ds_volno, ds_totread, ds_volpart,
	    ds_skippart, ds_bufsize, ds_toc->nparts, ds_toc->maxsiz,
	    ds_toc->volnos);
}

int
ds_getinfo(char *string)
{
	ds_toc = (struct dstoc *)calloc(1, sizeof (struct dstoc));
	(void) sscanf(string, "%d %d %d %d %d %d %d %d %d %ld %[ 0-9]",
	    &ds_fd, &ds_realfd, &ds_volcnt, &ds_volno, &ds_totread,
	    &ds_volpart, &ds_skippart, &ds_bufsize, &ds_toc->nparts,
	    &ds_toc->maxsiz, ds_toc->volnos);
	ds_pkginit();
	return (ds_toc->nparts);
}

/*
 * Return true if the file descriptor (ds_fd) is open on the package stream.
 */
boolean_t
ds_fd_open(void)
{
	return (ds_fd >= 0 ? B_TRUE : B_FALSE);
}

/*
 * Read the source device. Acquire the header data and check it for validity.
 */
int
ds_init(char *device, char **pkg, char *norewind)
{
	struct dstoc *tail, *toc_pt;
	char	*ret;
	char	cmd[CMDSIZ];
	char	line[LSIZE+1];
	int	i, n, count = 0, header_size = BLK_SIZE;

	if (!ds_header) { 	/* If the header hasn't been read yet */
		if (ds_fd >= 0)
			(void) ds_close(0);

		/* always start with rewind device */
		if ((ds_fd = open(device, O_RDONLY | O_LARGEFILE)) < 0) {
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_OPEN), device, errno);
			return (-1);
		}

		/* allocate room for the header equivalent to a block */
		if ((ds_header = (char *)calloc(BLK_SIZE, 1)) == NULL) {
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_MEM));
			return (-1);
		}

		/* initialize the device */
		if (ds_ginit(device) < 0) {
			(void) ds_close(0);
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_OPEN), device, errno);
			return (-1);
		}

		/* read a logical block from the source device */
		if (read(ds_fd, ds_header, BLK_SIZE) != BLK_SIZE) {
			rpterr();
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_TOC));
			(void) ds_close(0);
			return (-1);
		}

		/*
		 * This loop scans the medium for the start of the header.
		 * If the above read worked, we skip this. If it did't, this
		 * loop will retry the read ten times looking for the header
		 * marker string.
		 */
		while (strncmp(ds_header, HDR_PREFIX, 20) != 0) {
			/* only ten tries iff the device rewinds */
			if (!norewind || count++ > 10) {
				progerr(pkg_gt(ERR_UNPACK));
				logerr(pkg_gt(MSG_TOC));
				(void) ds_close(0);
				return (-1);
			}

			/* read through to the last block */
			if (count > 1)
				while (read(ds_fd, ds_header, BLK_SIZE) > 0)
					;

			/* then close the device */
			(void) ds_close(0);

			/* and reopen it */
			if ((ds_fd = open(norewind, O_RDONLY
			    | O_LARGEFILE)) < 0) {
				progerr(pkg_gt(ERR_UNPACK));
				logerr(pkg_gt(MSG_OPEN), device, errno);
				(void) free(ds_header);
				return (-1);
			}

			/* initialize the device */
			if (ds_ginit(device) < 0) {
				(void) ds_close(0);
				progerr(pkg_gt(ERR_UNPACK));
				logerr(pkg_gt(MSG_OPEN), device, errno);
				return (-1);
			}

			/* read the block again */
			if (read(ds_fd, ds_header, BLK_SIZE) != BLK_SIZE) {
				rpterr();
				progerr(pkg_gt(ERR_UNPACK));
				logerr(pkg_gt(MSG_TOC));
				(void) ds_close(0);
				return (-1);
			}
		}

		/* Now keep scanning until the whole header is in place. */
		while (strstr(ds_header, HDR_SUFFIX) == NULL) {
			/* We need a bigger buffer */
			if ((ds_header = (char *)realloc(ds_header,
			    header_size + BLK_SIZE)) == NULL) {
				progerr(pkg_gt(ERR_UNPACK));
				logerr(pkg_gt(MSG_MEM));
				(void) ds_close(0);
				return (1);
			}

			/* clear the new memory */
			(void) memset(ds_header + header_size, '\0',
			    BLK_SIZE);


			/* read a logical block from the source device */
			if (read(ds_fd, ds_header + header_size, BLK_SIZE) !=
			    BLK_SIZE) {
				rpterr();
				progerr(pkg_gt(ERR_UNPACK));
				logerr(pkg_gt(MSG_TOC));
				(void) ds_close(0);
				return (-1);
			} else
				header_size += BLK_SIZE;	/* new size */
		}

		/*
		 * remember rewind device for ds_close to rewind at
		 * close
		 */
		if (count >= 1)
			ds_device = device;
		ds_headsize = header_size;

	}

	pds_header = ds_header;

	/* save raw copy of header for later use in BIO_dump_header */
	if ((ds_header_raw = (char *)malloc(header_size)) == NULL) {
		progerr(pkg_gt(ERR_UNPACK));
		logerr(pkg_gt(MSG_MEM));
		(void) ds_close(0);
		return (1);
	}
	memcpy(ds_header_raw, ds_header, header_size);

	/* read datastream table of contents */
	ds_head = tail = (struct dstoc *)0;
	ds_volcnt = 1;

	while (ret = ds_gets(line, LSIZE)) {
		if (strcmp(line, HDR_SUFFIX) == 0)
			break;
		if (!line[0] || line[0] == '#')
			continue;
		toc_pt = (struct dstoc *)calloc(1, sizeof (struct dstoc));
		if (!toc_pt) {
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_MEM));
			ecleanup();
			(void) free(ds_header);
			return (-1);
		}
		if (sscanf(line, "%s %d %ld %[ 0-9]", toc_pt->pkg,
		    &toc_pt->nparts, &toc_pt->maxsiz, toc_pt->volnos) < 3) {
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_TOC));
			free(toc_pt);
			(void) free(ds_header);
			ecleanup();
			return (-1);
		}
		if (tail) {
			tail->next = toc_pt;
			tail = toc_pt;
		} else
			ds_head = tail = toc_pt;
		ds_volcnt += ds_volsum(toc_pt);
	}
	if (!ret) {
		progerr(pkg_gt(ERR_UNPACK));
		logerr(pkg_gt(MSG_TOC));
		(void) free(ds_header);
		return (-1);
	}
	sighold(SIGINT);
	sigrelse(SIGINT);
	if (!ds_head) {
		progerr(pkg_gt(ERR_UNPACK));
		logerr(pkg_gt(MSG_EMPTY));
		(void) free(ds_header);
		return (-1);
	}
	/* this could break, thanks to cpio command limit */
	(void) sprintf(cmd, "%s -icdumD -C %d", CPIOPROC, (int)BLK_SIZE);
	n = 0;
	for (i = 0; pkg[i]; i++) {
		if (strcmp(pkg[i], "all") == 0)
			continue;
		if (n == 0) {
			strcat(cmd, " ");
			n = 1;
		}
		strlcat(cmd, pkg[i], CMDSIZ);
		strlcat(cmd, "'/*' ", CMDSIZ);

		/* extract signature too, if present. */
		strlcat(cmd, SIGNATURE_FILENAME, CMDSIZ);
		strlcat(cmd, " ", CMDSIZ);
	}

	/*
	 * if we are extracting all packages (pkgs == NULL),
	 * signature will automatically be extracted
	 */
	if (n = esystem(cmd, ds_fd, -1)) {
		rpterr();
		progerr(pkg_gt(ERR_UNPACK));
		logerr(pkg_gt(MSG_CMDFAIL), cmd, n);
		(void) free(ds_header);
		return (-1);
	}

	ds_toc = ds_head;
	ds_totread = 0;
	ds_volno = 1;
	return (0);
}

int
ds_findpkg(char *device, char *pkg)
{
	char	*pkglist[2];
	int	nskip, ods_volpart;

	if (ds_head == NULL) {
		pkglist[0] = pkg;
		pkglist[1] = NULL;
		if (ds_init(device, pkglist, NULL))
			return (-1);
	}

	if (!pkg || pkgnmchk(pkg, "all", 0)) {
		progerr(pkg_gt(ERR_UNPACK));
		logerr(pkg_gt(MSG_PKGNAME));
		return (-1);
	}

	nskip = 0;
	ds_volno = 1;
	ds_volpart = 0;
	ds_toc = ds_head;
	while (ds_toc) {
		if (strcmp(ds_toc->pkg, pkg) == 0)
			break;
		nskip += ds_toc->nparts;
		ds_volno += ds_volsum(ds_toc);
		ds_toc = ds_toc->next;
	}
	if (!ds_toc) {
		progerr(pkg_gt(ERR_UNPACK));
		logerr(pkg_gt(MSG_NOPKG), pkg);
		return (-1);
	}

	ds_pkginit();
	ds_skippart = 0;
	if (ds_curpartcnt > 0) {
		ods_volpart = ds_volpart;
		/*
		 * skip past archives belonging to last package on current
		 * volume
		 */
		if (ds_volpart > 0 && ds_getnextvol(device))
			return (-1);
		ds_totread = nskip - ods_volpart;
		if (ds_skip(device, ods_volpart))
			return (-1);
	} else if (ds_curpartcnt < 0) {
		if (ds_skip(device, nskip - ds_totread))
			return (-1);
	} else
		ds_totread = nskip;
	ds_read = 0;
	return (ds_nparts);
}

/*
 * Get datastream part
 * Call for first part should be preceded by
 * call to ds_findpkg
 */

int
ds_getpkg(char *device, int n, char *dstdir)
{
	struct statvfs svfsb;
	unsigned long long free_blocks;

	if (ds_read >= ds_nparts)
		return (2);

	if (ds_read == n)
		return (0);
	else if ((ds_read > n) || (n > ds_nparts))
		return (2);

	if (ds_maxsiz > 0) {
		if (statvfs(".", &svfsb)) {
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_STATFS), errno);
			return (-1);
		}
		free_blocks = (((long)svfsb.f_frsize > 0) ?
			    howmany(svfsb.f_frsize, DEV_BSIZE) :
			    howmany(svfsb.f_bsize, DEV_BSIZE)) * svfsb.f_bfree;
		if ((ds_maxsiz + 50) > free_blocks) {
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_NOSPACE), ds_maxsiz+50, free_blocks);
			return (-1);
		}
	}
	return (ds_next(device, dstdir));
}

static int
ds_getnextvol(char *device)
{
	char prompt[128];
	int n;

	if (ds_close(0))
		return (-1);
	(void) sprintf(prompt,
	    pkg_gt("Insert %%v %d of %d into %%p"),
	    ds_volno, ds_volcnt);
	if ((ds_fd = open(device, O_RDONLY | O_LARGEFILE)) < 0)
		return (-1);
	if (ds_ginit(device) < 0) {
		(void) ds_close(0);
		return (-1);
	}
	ds_volpart = 0;
	return (0);
}

/*
 * called by ds_findpkg to skip past archives for unwanted packages
 * in current volume
 */
static int
ds_skip(char *device, int nskip)
{
	char	cmd[CMDSIZ];
	int	n, onskip = nskip;

	while (nskip--) {
		/* skip this one */
		(void) sprintf(cmd, "%s -ictD -C %d > /dev/null",
		    CPIOPROC, (int)BLK_SIZE);
		if (n = esystem(cmd, ds_fd, -1)) {
			rpterr();
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_CMDFAIL), cmd, n);
			nskip = onskip;
			if (ds_volno == 1 || ds_volpart > 0)
				return (n);
			if (n = ds_getnextvol(device))
				return (n);
		}
	}
	ds_totread += onskip;
	ds_volpart = onskip;
	ds_skippart = onskip;
	return (0);
}

/* skip to end of package if necessary */
void
ds_skiptoend(char *device)
{
	if (ds_read < ds_nparts && ds_curpartcnt < 0)
		(void) ds_skip(device, ds_nparts - ds_read);
}

int
ds_next(char *device, char *instdir)
{
	char	cmd[CMDSIZ], tmpvol[128];
	int	nparts, n, index;

	/*CONSTCOND*/
	while (1) {
		if (ds_read + 1 > ds_curpartcnt && ds_curpartcnt >= 0) {
			ds_volno++;
			if (n = ds_getnextvol(device))
				return (n);
			(void) sscanf(ds_volnos, "%d %[ 0-9]", &index, tmpvol);
			(void) strcpy(ds_volnos, tmpvol);
			ds_curpartcnt += index;
		}
		(void) sprintf(cmd, "%s -icdumD -C %d",
		    CPIOPROC, (int)BLK_SIZE);
		if (n = esystem(cmd, ds_fd, -1)) {
			rpterr();
			progerr(pkg_gt(ERR_UNPACK));
			logerr(pkg_gt(MSG_CMDFAIL), cmd, n);
		}
		if (ds_read == 0)
			nparts = 0;
		else
			nparts = ds_toc->nparts;
		if (n || (n = ckvolseq(instdir, ds_read + 1, nparts))) {
			if (ds_volno == 1 || ds_volpart > ds_skippart)
				return (-1);

			if (n = ds_getnextvol(device))
				return (n);
			continue;
		}
		ds_read++;
		ds_totread++;
		ds_volpart++;

		return (0);
	}
	/*NOTREACHED*/
}

/*
 * Name:		BIO_ds_dump
 * Description:	Dumps all data from the static 'ds_fd' file handle into
 *		the supplied BIO.
 *
 * Arguments:	err - where to record any errors.
 *		device - Description of device being dumped into,
 *			for error reporting
 *		bio - BIO object to dump data into
 *
 * Returns :	zero - successfully dumped all data to EOF
 *		non-zero - some failure occurred.
 */
int
BIO_ds_dump(PKG_ERR *err, char *device, BIO *bio)
{
#ifdef	USE_KEYSTORE
	int	amtread;
	char	readbuf[BLK_SIZE];

	/*
	 * note this will read to the end of the device, so it won't
	 * work for character devices since we don't know when the
	 * end of the CPIO archive is
	 */
	while ((amtread = read(ds_fd, readbuf, BLK_SIZE)) != 0) {
		if (BIO_write(bio, readbuf, amtread) != amtread) {
			pkgerr_add(err, PKGERR_WRITE, ERR_WRITE, device,
			    ERR_error_string(ERR_get_error(), NULL));
			return (1);
		}
	}

	return (0);
	/*NOTREACHED*/
#else	/* !USE_KEYSTORE */
	return (1);
#endif	/* !USE_KEYSTORE */
}


/*
 * Name:		BIO_ds_dump_header
 * Description:	Dumps all ds_headsize bytes from the
 *		static 'ds_header_raw' character array
 *		to the supplied BIO.
 *
 * Arguments:	err - where to record any errors.
 *		bio - BIO object to dump data into
 *
 * Returns :	zero - successfully dumped all raw
 *		header characters
 *		non-zero - some failure occurred.
 */
int
BIO_ds_dump_header(PKG_ERR *err, BIO *bio)
{

#ifdef	USE_KEYSTORE
	char	zeros[BLK_SIZE];

	memset(zeros, 0, BLK_SIZE);

	if (BIO_write(bio, ds_header_raw, ds_headsize) != ds_headsize) {
		pkgerr_add(err, PKGERR_WRITE, ERR_WRITE, "bio",
		    ERR_error_string(ERR_get_error(), NULL));
		return (1);
	}

	return (0);
#else	/* !USE_KEYSTORE */
	return (1);
#endif	/* !USE_KEYSTORE */
}

/*
 * ds_ginit: Determine the device being accessed, set the buffer size,
 * and perform any device specific initialization.  For the 3B2,
 * a device with major number of 17 (0x11) is an internal hard disk,
 * unless the minor number is 128 (0x80) in which case it is an internal
 * floppy disk.  Otherwise, get the system configuration
 * table and check it by comparing slot numbers to major numbers.
 * For the special case of the 3B2 CTC several unusual things must be done.
 * To enable
 * streaming mode on the CTC, the file descriptor must be closed, re-opened
 * (with O_RDWR and O_CTSPECIAL flags set), the STREAMON ioctl(2) command
 * issued, and the file descriptor re-re-opened either read-only or write_only.
 */

int
ds_ginit(char *device)
{
	ds_bufsize = BLK_SIZE;
	return (ds_bufsize);
}

int
ds_close(int pkgendflg)
{
	int n, ret = 0;

	if (pkgendflg) {
		if (ds_header)
			(void) free(ds_header);
		ds_header = (char *)NULL;
		ds_totread = 0;
	}

	if (ds_pp) {
		(void) pclose(ds_pp);
		ds_pp = 0;
		(void) close(ds_realfd);
		ds_realfd = -1;
		ds_fd = -1;
	} else if (ds_fd >= 0) {
		(void) close(ds_fd);
		ds_fd = -1;
	}

	if (ds_device) {
		/* rewind device */
		if ((n = open(ds_device, 0)) >= 0)
			(void) close(n);
		ds_device = NULL;
	}
	return (ret);
}
