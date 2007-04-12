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

#include "initng.h"

#define _GNU_SOURCE
#include <fnmatch.h>

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include "initng_string_tools.h"
#include "initng_toolbox.h"


/* function strips "test/go/th" to "th" */
const char *st_strip_path(const char *string)
{
	char *ret = NULL;

	assert(string);

	/* get the last '/' char in string */
	if (!(ret = strrchr(string, '/')))
		return (string);

	/* skip to char after the last '/' */
	ret++;
	return (ret);
}

/* function strips "test/go/th" to "test/go" */
char *st_get_path(const char *string)
{
	char *last_slash;

	assert(string);

	/* get the last_slash position */
	if (!(last_slash = strrchr(string, '/')))
		return (i_strdup(string));

	/* ok return a copy of that string */
	return (i_strndup(string, (int) (last_slash - string)));
}

/* function strips "test/go/th" to "test/go" and then to "test" */
int st_strip_end(char **string)
{

	assert(string);
	assert(*string);

	/* go to last '/' */
	char *end = strrchr(*string, '/');

	if (end)
	{
		end[0] = '\0';
		return (TRUE);
	}
	return (FALSE);
}
