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
#include <stdlib.h>
#include <string.h>


#define CHUNK	16

/** Re-links a string to a argv-like string-array.
 * Example:   /bin/executable --ham -flt   --moo hoho  lalala\0
 *       =>   /bin/executable\0--ham\0-flt\0  --moo\0hoho\0 lalala\0
 *    argv:   ^[0]             ^[1]   ^[2]    ^[3]   ^[4]   ^[5]
 *    argc: 6
 *
 * string: String to split.
 * delim : Char that splits the strings.
 * argc  : Pointer, sets the number of splits done.
 * ofs   : Offset, start from this arg to fill.
 * @idea DEac-
 * @author TheLich
 * @got_it_working powerj
 * @improved_it ismael
 */

char **initng_string_split_delim(const char *string, const char *delim,
				 size_t *argc, int ofs)
{
	/* TODO: remove the argc and ofs arguments */

	char *dest;
	size_t len;

	char **array;
	size_t array_size;
	size_t i = 0;

	char **saveptr, *tok;
	char *tmp;

	if (!string)
		return NULL;

	string += ofs; /* TODO: REMOVE */

	/* skip any inital character in delim, to make sure dest[0]
	 * will be the start of the string */
	while (strcspn(string, delim) == 0) {
		string++;
	}

	len = strlen(string) + 1;
	dest = initng_toolbox_calloc(1, len);
	memcpy(dest, string, len);

	array = initng_toolbox_calloc(1, CHUNK * sizeof(char *));
	array_size = CHUNK;

	array[i++] = strtok_r(dest, delim, saveptr);
	
	while (tok = strtok_r(NULL, delim, saveptr) {
		array[i++] = tok;
		if (i % CHUNK == 0) {
			array_size += CHUNK;
			array = initng_toolbox_realloc(array, array_size *
							      sizeof(char *));
		}
	}

	array[i++] = NULL;

	tmp = initng_toolbox_realloc(dest, len + i * sizeof(char *));
	if (tmp != dest) {
		for (int j = 0; j < i; j++) {
			array[j] += tmp - dest;
		}
		dest = tmp;
	}
	
	memcpy(&dest[len], array, i);

	free(array);

	if (argc) /* TODO: REMOVE */
		*argc = i - 1;

	return &dest[len];
}


void initng_string_split_delim_free(char **strs)
{
	free(strs[0]);
}
