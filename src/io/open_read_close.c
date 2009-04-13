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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

char *initng_io_readwhole(const char *path)
{
	int fd;
	struct stat stat_buf;
	int res;		/* Result of read */
	char *buf;

	fd = open(filename, O_RDONLY);	/* Open config file. */

	if (fd == -1) {
		D_("error opening %s: %s (%d)\n",
		   filename, strerror(errno), errno);
		return NULL;
	}

	if (fstat(fd, &stat_buf) == -1) {
		D_("error getting %s file size: %s (%d)\n",
		   filename, strerror(errno), errno);
		close(fd);
		return NULL;
	}

	/* Allocate a file buffer */

	buf = (char *)initng_toolbox_calloc((stat_buf.st_size + 1), 1);

	/* Read whole file */

	res = read(fd, buf, stat_buf.st_size);

	if (res == -1) {
		F_("error reading %s: %s (%d)\n",
		   filename, strerror(errno), errno);
		goto error;
	} else if (res != stat_buf.st_size) {
		F_("read %d instead of %d bytes from %s\n",
		   (int)res, (int)stat_buf.st_size, filename);
		goto error;
	}

	/* Normally we wouldn't care about this, but as this is init(ng)? */
	if (close(fd) < 0) {
		F_("error closing %s: %s (%d)\n",
		   filename, strerror(errno), errno);
		goto error;
	}

	buf[stat_buf.st_size] = '\0';	/* nil-terminate buffer */

	return buf;

error:	free(buf);
	close(fd);
	return NULL;
}

