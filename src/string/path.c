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

#include <fnmatch.h>

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

/* function strips "test/go/th" to "th" */
const char *initng_string_basename(const char *string)
{
	char *ret;

	assert(string);

	/* get the last '/' char in string */
	if (!(ret = strrchr(string, '/')))
		return string;

	/* skip to char after the last '/' */
	return ++ret;
}

/* function strips "test/go/th" to "test/go" */
char *initng_string_dirname(const char *string)
{
	char *last_slash;

	assert(string);

	/* get the last_slash position */
	if (!(last_slash = strrchr(string, '/')))
		return initng_toolbox_strdup(string);

	/* ok return a copy of that string */
	return initng_toolbox_strndup(string, (int)(last_slash - string));
}

/* function strips "test/go/th" to "test/go" and then to "test" */
int initng_string_strip_end(char **string)
{

	char *end;
	assert(string);
	assert(*string);

	/* go to last '/' */
	end = strrchr(*string, '/');

	if (end) {
		end[0] = '\0';
		return TRUE;
	}
	return FALSE;
}
