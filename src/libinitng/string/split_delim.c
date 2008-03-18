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
				 size_t * argc, int ofs)
{
	int len;
	char **array;
	size_t i = 0;

	if (!string)
		return NULL;

	array = (char **)initng_toolbox_calloc(1, sizeof(char *));

	while (string[ofs] != '\0') {
		len = strcspn(string + ofs, delim);
		if (len != 0) {
			array = (char **)initng_toolbox_realloc(array,
						sizeof(char *) * (i + 2));
			array[i] = initng_toolbox_calloc(1, len);
			memcpy(array[i], string + ofs, len);
			i++;
		} else {
			len = 1;
		}

		ofs += len;
	}

	array[i] = NULL;

	if (argc)
		*argc = i;

	return array;
}

void initng_string_split_delim_free(char **strs)
{
	int i;

	if (!strs)
		return;

	for (i = 0; strs[i] != NULL; i++)
		free(strs[i]);

	free(strs);
}
