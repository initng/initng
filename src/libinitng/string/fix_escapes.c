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


void fix_escapes(char * str)
{
	int s, d;

	if (!str)
		return;

	for (s = 0, d = 0; str[s] != '\0'; s++, d++)
	{
		if (str[s] != '\\') {
			if (s != d)
				str[d] = str[s];
			continue;
		}

		s++;
		switch(str[s])
		{
			case 'a':
				str[d] = '\a';
				break;

			case 'b':
				str[d] = '\b';
				break;

			case 'f':
				str[d] = '\f';
				break;

			case 'n':
				str[d] = '\n';
				break;

			case 'r':
				str[d] = '\r';
				break;

			case 't':
				str[d] = '\t';
				break;

			case 'v':
				str[d] = '\v';
				break;

			default:
				str[d] = str[s];
		}
	}

	str[d] = '\0';
}
