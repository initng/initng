/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <initng.h>

#define _GNU_SOURCE
#include <fnmatch.h>

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>


void initng_string_replace(char * dest, char * src, const char * n,
                           const char * r)
{
	char *p;
	char *d = dest;
	char *last = src;
	int nlen = strlen(n);
	int rlen = strlen(r);

	while ((p = strstr(last, n))) {
		memmove(d, last, p - last);
		d += p - last;
		memmove(d, r, rlen);
		d += rlen;
		last = p + nlen;
	}

	if (d != last)
		memmove(d, last, strlen(last));
}
