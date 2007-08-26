/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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
#include <stdlib.h>		/* free() exit() */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>		/* vastart() vaend() */

/* in init, a calloc must never fail! */
/* void *initng_calloc2(size_t nmemb, size_t size, const char *func, int line) */
void *initng_toolbox_calloc(size_t nmemb, size_t size)
{
	void *alloced;

	/*D_("function %s() line %i allocating %i bytes\n", func, line,
	   nmemb * size); */
	while ((alloced = calloc(nmemb, size)) == NULL) {
		F_("Out of memory, trying again in 1 second\n");
		sleep(1);
	}

	return alloced;
}

/* in init, a realloc must never fail! */
void *initng_toolbox_realloc2(void *ptr, size_t size, const char *func,
			      int line)
{
	void *alloced;

	while ((alloced = realloc(ptr, size)) == NULL) {
		F_("%s(): line %i Out of memory, trying again in 1 second\n",
		   func, line);
		sleep(1);
	}

	return alloced;
}

/* in init, a strdup must never fail! */
char *initng_toolbox_strdup2(const char *s, const char *func, int line)
{
	char *alloced;

	while ((alloced = strdup(s)) == NULL) {
		F_("%s(): line %i Out of memory, trying again in 1 second\n",
		   func, line);
		sleep(1);
	}

	return alloced;
}

/* in init, a strndup must never fail! */
char *initng_toolbox_strndup2(const char *s, size_t n, const char *func,
			      int line)
{
	char *alloced;

	while ((alloced = strndup(s, n)) == NULL) {
		F_("%s(): line %i Out of memory, trying again in 1 second\n",
		   func, line);
		sleep(1);
	}

	return alloced;
}
