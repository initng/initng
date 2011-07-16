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

#include <stdlib.h>		/* free() exit() */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>		/* vastart() vaend() */

/* stolen from sysvinit */

/* Set the name of the process, so it looks good in "ps" output. */
/* The new name cannot be longer than the space available, which */
/* is the space for argv[] and env[]. */

/* returns the length of the new title, or 0 if it cannot be changed */

/*
 * See discussion of "Changing argv[0] under Linux." on Linux-Kernel
 * mailing list:
 * http://www.uwsg.iu.edu/hypermail/linux/kernel/0301.1/index.html#2368
 */
int initng_toolbox_set_proc_title(const char *fmt, ...)
{
	va_list ap;
	int len;

	va_start(ap, fmt);
	len = vsnprintf(g.Argv0, g.maxproclen, fmt, ap);
	va_end(ap);

	if (len >= g.maxproclen) {
		g.Argv0[g.maxproclen] = 0;
		len = g.maxproclen - 1;
	}

	return len;
}
