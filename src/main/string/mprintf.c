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


int initng_string_mprintf(char **p, const char *format, ...)
{
	va_list arg;		/* used for the variable
				 * lists */
	int len = 0;		/* will contain lent for
				 * current string */
	int add_len = 0;	/* This mutch more strings are
				 * we gonna alloc for */

	/* printf("\n\ninitng_string_mprintf(%s);\n", format); */

	/* count old chars */
	if (*p)
		len = strlen(*p);

	/*
	 * format contains the minimum needed chars that
	 * are gonna be used, so we set that value and try
	 * with that.
	 */
	add_len = strlen(format) + 1;

	/*
	 * Now realloc the memoryspace containing the
	 * string so that the new apending string will
	 * have room.
	 * Also have a check that it succeds.
	 */
	/*printf("Changing size to %i\n", len + add_len); */
	{
		char *tmp = realloc(*p, sizeof(char [len + add_len]));
		if (!tmp)
			return -1;

		*p = tmp;
	}

	/* Ok, have a try until vsnprintf succeds */
	while (1) {
		int done;

		/* start filling the newly allocaded area */
		va_start(arg, format);
		done = vsnprintf((*p) + len, add_len, format, arg);

		va_end(arg);

		/* check if that was enouth */
		if (done > -1 && done < add_len) {
			/*printf("GOOD: done : %i, len: %i\n", done, add_len); */

			/* Ok return happily */
			return done;
		}

		/* try increase it a bit. */
		add_len = (done < 0 ? add_len >> 1 : done + 1);

		char *tmp = realloc(*p, len + add_len);
		if (!tmp)
			return -1;
		*p = tmp;

	}

	/* will never go here */
	return -1;
}
