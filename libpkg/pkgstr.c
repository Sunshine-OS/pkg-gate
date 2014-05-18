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
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */

#ifndef lint
/*	from OpenSolaris "pkgstr.c	1.2	06/02/27 SMI"	*/

/*
 * Portions Copyright (c) 2007 Gunnar Ritter, Freiburg i. Br., Germany
 *
 * Sccsid @(#)pkgstr.c	1.2 (gritter) 2/24/07
 */
#endif

/*
 * Module:	pkgstr.c
 * Synopsis:	general string services
 * Taxonomy:	project private
 * Debug Flag:	str
 * Description:
 *
 *   This module implements general string utility services
 *
 * Public Methods:
 *
 *   pkgstrContainsToken - Determine if a string contains a specified token
 *   pkgstrGetToken_r - Get a token from a string into a fixed buffer
 *   pkgstrPrintf - Create a string from a printf style format and arguments
 *   pkgstrPrintf_r - Create a string from a printf style format and arguments
 *			into a fixed buffer
 */

/*
 * Unix Includes
 */

#define	__EXTENSIONS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <limits.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <libintl.h>
#include <ctype.h>
#include <unistd.h>
#include <strings.h>
#include <stdarg.h>

/*
 * pkglib Includes
 */

#include <pkglib.h>
#include <pkgstrct.h>
#include <libintl.h>
#include "pkglocale.h"

/*
 * External definitions
 */

/*
 * Public methods
 */

/*
 * Name:	pkgstrPrintf_r
 * Synopsis:	Create string from printf style format and arguments
 * Description:	Call to convert a printf style format and arguments into a
 *		string of characters placed in allocated storage
 * Arguments:	a_buf - [RO, *RW] - (char *)
 *			- Pointer to buffer used as storage space for the
 *			  returned string created
 *		a_bufLen - [RO, *RO] - (int)
 *			- Size of 'a_buf' in bytes - a maximum of 'a_bufLen-1'
 *			  bytes will be placed in 'a_buf' - the returned
 *			  string is always null terminated
 *		a_format - [RO, RO*] (char *)
 *			printf-style format for string to be formatted
 *		VARG_LIST - [RO] (?)
 *			arguments as appropriate to 'format' specified
 * Returns:	void
 */

/*PRINTFLIKE3*/
void
pkgstrPrintf_r(char *a_buf, int a_bufLen, char *a_format, ...)
{
	va_list		ap;
	size_t		vres = 0;

	/* entry assertions */

	assert(a_format != (char *)NULL);
	assert(*a_format != '\0');
	assert(a_buf != (char *)NULL);
	assert(a_bufLen > 1);

	/* generate the results of the printf conversion */

	va_start(ap, a_format);
	vres = vsnprintf(a_buf, a_bufLen-1, a_format, ap);
	va_end(ap);

	assert(vres > 0);
	assert(vres < a_bufLen);

	a_buf[a_bufLen-1] = '\0';
}

/*
 * Name:	pkgstrPrintf
 * Synopsis:	Create string from printf style format and arguments
 * Description:	Call to convert a printf style format and arguments into a
 *		string of characters placed in allocated storage
 * Arguments:	format - [RO, RO*] (char *)
 *			printf-style format for string to be formatted
 *		VARG_LIST - [RO] (?)
 *			arguments as appropriate to 'format' specified
 * Returns:	char *
 *			A string representing the printf conversion results
 * NOTE:    	Any string returned is placed in new storage for the
 *		calling method. The caller must use 'free' to dispose
 *		of the storage once the string is no longer needed.
 * Errors:	If the string cannot be created, the process exits
 */

/*PRINTFLIKE1*/
char *
pkgstrPrintf(char *a_format, ...)
{
	va_list		ap;
	size_t		vres = 0;
	char		bfr[1];
	char		*rstr = (char *)NULL;

	/* entry assertions */

	assert(a_format != (char *)NULL);
	assert(*a_format != '\0');

	/* determine size of the message in bytes */

	va_start(ap, a_format);
	vres = vsnprintf(bfr, 1, a_format, ap);
	va_end(ap);

	assert(vres > 0);
	assert(vres < LINE_MAX);

	/* allocate storage to hold the message */

	rstr = (char *)calloc(1, vres+2);
	assert(rstr != (char *)NULL);
	if (rstr == (char *)NULL) {
		return ((char *)NULL);
	}

	/* generate the results of the printf conversion */

	va_start(ap, a_format);
	vres = vsnprintf(rstr, vres+1, a_format, ap);
	va_end(ap);

	assert(vres > 0);
	assert(vres < LINE_MAX);
	assert(*rstr != '\0');

	/* return the results */

	return (rstr);
}

/*
 * Name:	pkgstrGetToken
 * Synopsis:	Get separator delimited token from a string into a fixed buffer
 * Description:	Given a string and a list of one or more separators,
 *		return the position specified token (sequence of one or
 *		more characters that do not include any of the separators)
 *		into a specified buffer of a fixed maximum size
 * Arguments:	r_sep - [*RW] - (char *)
 *			- separator that ended the token returned
 *			- NOTE: this is a pointer to a "char", e.g.:
 *				- char a;
 *				- pkgstrGetToken(&a, ...)
 *		a_string - [RO, *RO] - (char *)
 *			- pointer to string to extract token from
 *		a_index - [RO, *RO] - (int)
 *			- Index of token to return; '0' is first matching
 *			  token, '1' is second matching token, etc.
 *		a_separators - [RO, *RO] - (char *)
 *			- String containing one or more characters that
 *			  can separate one "token" from another
 *		a_buf - [RO, *RW] - (char *)
 *			- Pointer to buffer used as storage space for the
 *			  returned token - the returned token is always
 *			  null terminated
 *			  a_buf[0] == '\0' - no token meeting criteria found
 *			  a_buf[0] != '\0' - token meeting criteria returned
 *		a_bufLen - [RO, *RO] - (int)
 *			- Size of 'a_buf' in bytes - a maximum of 'a_bufLen-1'
 *			  bytes will be placed in 'a_buf' - the returned
 *			  token is always null terminated
 * Returns:	void
 */

void
pkgstrGetToken_r(char *r_sep, char *a_string, int a_index,
	char *a_separators, char *a_buf, int a_bufLen)
{
	char	*p;
	char	*q;
	char	*lasts;

	/* entry assertions */

	assert(a_string != (char *)NULL);
	assert(a_index >= 0);
	assert(a_separators != (char *)NULL);
	assert(*a_separators != '\0');
	assert(a_buf != (char *)NULL);
	assert(a_bufLen > 0);

	/* reset returned separator */

	if (r_sep != (char *)NULL) {
		*r_sep = '\0';
	}

	/* zero out contents of return buffer */

	bzero(a_buf, a_bufLen);

	/* duplicate original string before breaking down into tokens */

	p = strdup(a_string);
	assert(p != (char *)NULL);
	if (p == (char *)NULL) {
		return;
	}
	lasts = p;

	/* scan for separators and return 'index'th token found */

	while (q = strtok_r((char *)NULL, a_separators, &lasts)) {
		/* retrieve separator if requested */

		if (r_sep != (char *)NULL) {
			char	*x;
			x = strpbrk(a_string, a_separators);
			if (x) {
				*r_sep = *x;
			}
		}

		/* if this is the 'index'th token requested return it */

		if (a_index-- == 0) {
			/* copy as many characters as possible to return buf */

			(void) strncpy(a_buf, q, a_bufLen-1);
			break;
		}
	}

	/* free up copy of original input string */

	free(p);
}

/*
 * Name:	pkgstrContainsToken
 * Synopsis:	Does a given string contain a specified substring
 * Description:	Determine if a given substring exists in a larger string
 * Arguments:	a_string - [RO, *RO] - (char *)
 *			Pointer to string to look for substring in
 *		a_token - [RO, *RO] - (char *)
 *			Pointer to substring to look for in larger string
 * Results:	boolean_t
 *			B_TRUE - substring exists in larger string
 *			B_FALSE - substring does NOT exist in larger string
 * NOTE:	The substring must match on a "token" basis; that is, the
 *		substring must exist in the larger string delineated with
 *		either spaces or tabs to match.
 */

boolean_t
pkgstrContainsToken(char *a_string, char *a_token, char *a_separators)
{
	char	*lasts;
	char	*current;
	char	*p;

	/* entry assertions */

	assert(a_separators != (char *)NULL);
	assert(*a_separators != '\0');

	/* if token is not supplied, return false */

	if (a_token == (char *)NULL) {
		return (B_FALSE);
	}

	/* if no string provided, return false */

	if (a_string == (char *)NULL) {
		return (B_FALSE);
	}

	/* if string empty (zero length), return false */

	if (*a_string == '\0') {
		return (B_FALSE);
	}

	/* duplicate larger string because strtok_r changes it */

	p = strdup(a_string);
	assert(p != (char *)NULL);
	if (p == (char *)NULL) {
		return (B_FALSE);
	}

	lasts = p;

	/* scan each token looking for a match */

	while ((current = strtok_r((char *)NULL, a_separators, &lasts)) !=
			(char *)NULL) {
		if (streq(current, a_token)) {
			free(p);
			return (B_TRUE);
		}
	}

	/* free up temporary storage */

	free(p);

	/* not found */

	return (B_FALSE);
}
