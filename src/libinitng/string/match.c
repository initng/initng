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
 * a simple strncasecmp, which only returns TRUE or FALSE
 * it will also add the pointer of string, the value, to forward the
 * pointer to next word.
 */
int initng_string_cmp(char **string, const char *to_cmp)
{
	int chars = 0;

	assert(to_cmp);
	assert(string);

	chars = strlen(to_cmp);

	/* skip beginning first spaces */
	JUMP_SPACES(*string);
	/* this might be an "comp pare" */
	if ((*string)[0] == '"' && to_cmp[0] != '"')
		(*string)++;

	/* ok, strcasecmp this */
	if (strncasecmp((*string), to_cmp, chars) == 0) {
		/* increase the string pointer */
		(*string) += chars;
		return TRUE;
	}
	return FALSE;
}

/*
 * This search for a pattern.
 * If string is "net/eth0" then pattern "net/et*" and pattern "net/eth?"
 * should return true.
 * The number of '/' chars must be equal on string and pattern.
 */
 // examples:
 // string           pattern            do_match
 // net/eth0         net/et*            TRUE
 // net/eth1         net/eth?           TRUE
 // test/abc/def     test/*/*           TRUE
 // daemon/test      daemon/*/*         FALSE

int initng_string_match(const char *string, const char *pattern)
{
	int stringslash = 0;
	int patternslash = 0;
	const char *tmp;

	assert(string);
	assert(pattern);
	D_("matching string: \"%s\", to pattern: \"%s\"\n", string, pattern);

	/* do pattern matching only if service name does not contain
	 * wildcards */
	if (strchr(string, '*') || strchr(string, '?')) {
		F_("The string \"%s\" contains wildcards line '*' and '?'!\n",
		   string);
		return FALSE;
	}

#ifdef FNM_CASEFOLD
	/* check if its a match */
	if (fnmatch(pattern, string, FNM_CASEFOLD) != 0)
		return FALSE;
#else
	/* check if its a match */
	if (fnmatch(pattern, string, 0) != 0)
		return FALSE;
#endif

	/* count '/' in string */
	tmp = string - 1;
	while ((tmp = strchr(++tmp, '/')))
		stringslash++;
	/* count '/' in pattern */
	tmp = pattern - 1;
	while ((tmp = strchr(++tmp, '/')))
		patternslash++;

	/*D_("service_match(%s, %s): %i, %i\n", string, pattern, stringslash, patternslash); */

	/* use fnmatch that shud serve us well as these matches looks like filename matching */
	return (stringslash == patternslash);
}


/*
 * This match is simply dont that we take pattern, removes any leading
 * '*' or '?', and to a strstr on that string.
 */
int initng_string_match_in_service(const char *string, const char *pattern)
{
	char *copy;
	char *tmp;

	assert(string);
	assert(pattern);

	/* if the string contains a '/' it is looking for an exact match, so
	 * no searching here */
	if (strchr(pattern, '/'))
		return FALSE;

	/* remove starting wildcards */
	while (pattern[0] == '*' || pattern[0] == '?')
		pattern++;

	/* check if it matches */
	if (strstr(string, pattern))
		return TRUE;

	/* if pattern wont have a '*' or '?' there is no ida to continue */
	if (!strchr(pattern, '*') && !strchr(pattern, '?'))
		return FALSE;

	/* now copy pattern, and remove ending '*' && '?' */
	copy = initng_toolbox_strdup(pattern);
	assert(copy);

	/* Trunk for any '*' or '?' found */
	while ((tmp = strrchr(copy, '*')) || (tmp = strrchr(copy, '?')))
		tmp[0] = '\0';

	/* check again */
	tmp = strstr(string, copy);

	/* cleanup */
	free(copy);

	/* if tmp was set return TRUE */
	return (tmp != NULL);
}
