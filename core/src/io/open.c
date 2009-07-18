/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2008 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2008 Ismael Luceno <ismael.luceno@gmail.com>
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

#include <fcntl.h>
#include <initng/io.h>

int initng_io_open(const char *path, int flags)
{
	int fd;

	fd = open(path, flags);
	if (fd != -1) {
		initng_io_fdtrack(fd);
	}
	return fd;
}