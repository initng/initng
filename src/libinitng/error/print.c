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

int lock_error_printing = 0;

int initng_error_print(e_mt mt, const char *file, const char *func, int line,
					   const char *format, ...)
{
	assert(file);
	assert(func);
	assert(format);

	int delivered = FALSE;
	va_list arg;

	/* This lock is to make sure we don't get into an endless print loop */
	if (lock_error_printing == 1)
		return (0);

	/* put the lock, to avoid a circular bug */
	lock_error_printing = 1;

	/* start the variable list */
	va_start(arg, format);

	/* check for hooks */
	{
		s_event event;
		s_event_error_message_data data;

		event.event_type = &EVENT_ERROR_MESSAGE;
		event.data = &data;
		data.mt = mt;
		data.file = file;
		data.func = func;
		data.line = line;
		data.format = format;

		va_copy(data.arg, arg);

		if (initng_event_send(&event) == TRUE)
			delivered = TRUE;

		va_end(data.arg);
	}

	/* Print on failsafe if no hook is to listen. */
	if (!delivered)
	{
		va_list pass;

		va_copy(pass, arg);

		initng_failsafe_print_error(mt, file, func, line, format, pass);

		va_end(pass);
	}

	va_end(arg);

	lock_error_printing = 0;
	return (TRUE);
}
