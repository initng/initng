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

#ifndef INITNG_IO_H
#define INITNG_IO_H

#include <unistd.h>
#include <fcntl.h>

#include <initng/active_db.h>

char *initng_io_readwhole(const char *path);

inline static bool STILL_OPEN(int fd)
{
	return (fcntl(fd, F_GETFD) != -1);
}

inline static int initng_io_set_cloexec(int fd)
{
	return fcntl(fd, F_SETFD, fcntl(fd, F_GETFD, 0) | FD_CLOEXEC);
}

void initng_io_process_read_input(active_db_h * service, process_h * p,
				  pipe_h * pipe);
void initng_io_module_poll(int timeout);

int initng_io_set_cloexec(int fd);

#endif /* !defined(INITNG_IO_H) */
