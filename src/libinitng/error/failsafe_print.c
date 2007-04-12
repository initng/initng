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
#include <stdlib.h>							/* free() exit() */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include "initng_error.h"

#include "initng_global.h"
#include "initng_toolbox.h"

#include "initng_static_event_types.h"


static void initng_failsafe_print_error(e_mt mt, const char *file,
					const char *func, int line,
					const char *format, va_list arg)
{
	struct tm *ts;
	time_t t;

	t = time(0);
	ts = localtime(&t);


	switch (mt)
	{
		case MSG_FAIL:
			if (file && func)
				fprintf(stderr,
						"\n\n FAILSAFE ERROR ** \"%s\", %s() line %i:\n",
						file, func, line);
			fprintf(stderr, " %.2i:%.2i:%.2i -- FAIL:\t", ts->tm_hour,
					ts->tm_min, ts->tm_sec);
			break;
		case MSG_WARN:
			if (file && func)
				fprintf(stderr,
						"\n\n FAILSAFE ERROR ** \"%s\", %s() line %i:\n",
						file, func, line);
			fprintf(stderr, " %.2i:%.2i:%.2i -- WARN:\t", ts->tm_hour,
					ts->tm_min, ts->tm_sec);
			break;
		default:
			break;
	}

	vfprintf(stderr, format, arg);

}
