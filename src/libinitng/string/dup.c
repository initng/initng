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


/*
 * Senses what in the string, that is a word, (can be a
 * line with "this is one word", it will i_strdup it, and
 * add word length on string pointer
 */
char *initng_string_dup_next_word(const char **string)
{
	char *td = NULL;
	int i = 0;

	assert(string);

	/* skip beginning first spaces */
	JUMP_SPACES(*string);
	if (!(*string)[0] || (*string)[0] == '\n')
		return (NULL);

	/* handle '"' chars */
	if ((*string)[0] == '"')
	{
		(*string)++;
		i = strcspn(*string, "\"");
		if (i < 1)
			return (NULL);
		td = initng_toolbox_strndup(*string, i);
		(*string) += i;
		if (*string[0] == '"')
			(*string)++;
		return (td);
	}

	/* handle '{' '}' chars */
	if ((*string)[0] == '{')
	{
		(*string)++;
		i = strcspn(*string, "}");
		if (i < 1)
			return (NULL);
		td = initng_toolbox_strndup(*string, i);
		(*string) += i;
		if (*string[0] == '}')
			(*string)++;
		return (td);
	}

	/* or copy until space tab newline, ; or , */
	i = strcspn(*string, " \t\n;,");
	if (i < 1)
		return (NULL);
	/* copy string */
	td = initng_toolbox_strndup(*string, i);
	(*string) += i;
	return (td);
}

/*
 * This will dup, in the string, what is resolved as the line,
 * (stops with '\n', ';', '\0'), copy it and increment *string with len
 */
char *initng_string_dup_line(char **string)
{
	char *td = NULL;
	int i = 0;

	assert(string);

	/* skip beginning first spaces */
	JUMP_SPACES(*string);
	if (!(*string)[0] || (*string)[0] == '\n')
		return (NULL);

	/* handle '"' chars */
	if ((*string)[0] == '"')
	{
		(*string)++;
		i = strcspn(*string, "\"");
		if (i < 1)
			return (NULL);
		td = initng_toolbox_strndup(*string, i);
		(*string) += i;
		if (*string[0] == '"')
			(*string)++;
		return (td);
	}

	/* handle '{' '}' chars */
	if ((*string)[0] == '{')
	{
		(*string)++;
		i = strcspn(*string, "}");
		if (i < 1)
			return (NULL);
		td = initng_toolbox_strndup(*string, i);
		(*string) += i;
		if (*string[0] == '}')
			(*string)++;
		return (td);
	}

	/* or copy until space tab newline, ; or , */
	i = strcspn(*string, "\n;");
	if (i < 1)
		return (NULL);
	/* copy string */
	td = initng_toolbox_strndup(*string, i);
	(*string) += i;
	return (td);
}
