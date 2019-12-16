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

#include <time.h>

#include <initng.h>

/* Get monotonic time. */
int initng_gettime_monotonic(struct timeval *tval)
{
	struct timespec tspec = { 0 };
	int status = clock_gettime(CLOCK_MONOTONIC, &tspec);

	tval->tv_sec = tspec.tv_sec;
	tval->tv_usec = tspec.tv_nsec/1000;

	return status;
}
