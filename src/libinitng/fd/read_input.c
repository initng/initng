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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>							/* fcntl() */

#include <initng.h>
#include "local.h"

/*
 * This function is called when data is polled below,
 * or when a process is freed ( with flush_buffer set)
 */
void initng_fd_process_read_input(active_db_h * service, process_h * p,
                                  pipe_h * pi)
{
	int old_content_offset = pi->buffer_len;
	int read_ret = 0;
	char *tmp;

	D_("\ninitng_fd_process_read_input(%s, %s);\n", service->name,
	   p->pt->name);

	if (pi->pipe[0] <= 0) {
		F_("FIFO, can't be read! NOT OPEN!\n");
		return;
	}

	/* INITIATE
	 * if this are not set to out_pipe, if there is nothing to read.
	 * read() will block initng and sit down waiting for input.
	 */
	if (!pi->buffer) {
		/* initziate buffer fnctl */
		int fd_flags;

		fd_flags = fcntl(pi->pipe[0], F_GETFL, 0);
		fcntl(pi->pipe[0], F_SETFL, fd_flags | O_NONBLOCK);
	}

	/* read data from process, and continue again after a interrupt */
	do {
		errno = 0;

		/* OBSERVE, initng_toolbox_realloc may change the path to the
		 * data, so dont set buffer_pos to early */

		/* Make sure there is room for 100 more chars */
		D_(" %i (needed buffersize) > %i(current buffersize)\n",
		   pi->buffer_len + 100, pi->buffer_allocated);

		if (pi->buffer_len + 100 >= pi->buffer_allocated) {
			/* do a realloc */
			D_("Changing size of buffer %p from %i to: %i "
			   "bytes.\n", pi->buffer, pi->buffer_allocated,
			   pi->buffer_allocated + 100 + 1 );

			tmp = initng_toolbox_realloc(pi->buffer,
				(pi->buffer_allocated + 100 + 1));

			/* make sure realloc suceeded */
			if (tmp) {
				D_("pi->buffer changes from %p to %p.\n",
				   pi->buffer, tmp);

				pi->buffer = tmp;
				pi->buffer_allocated += 100;

				/*
				 * make sure it nulls, specially when
				 * initng_toolbox_realloc is run for the verry
				 * first time and maby there is nothing to get
				 * by read
				 */
				pi->buffer[pi->buffer_len] = '\0';
			} else {
				F_("realloc failed, possibly out of memory!\n");
				return;
			}
		}

		/* read the data */
		D_("Trying to read 100 chars:\n");
		read_ret = read(pi->pipe[0], &pi->buffer[pi->buffer_len], 100);
		/* printf("read_ret = %i  : \"%.100s\"\n", read_ret, read_pos); */
		D_("And got %i chars...\n", read_ret);

		/* make sure read does not return -1 */
		if (read_ret <= 0)
			break;

		/* increase buffer_len */
		pi->buffer_len += read_ret;

		/* make sure its nulled at end */
		pi->buffer[pi->buffer_len] = '\0';
	}
	/* if read_ret == 100, it migit be more to read, or it got
	 * interrupted. */
	while (read_ret >= 100 || errno == EINTR);

	D_("Done reading (read_ret=%i) (errno == %i).\n", read_ret, errno);

	/* make sure there is any */
	if (pi->buffer_len > old_content_offset) {
		D_("Calling plugins for new buffer content...");
		/* let all plugin take part of data */
		initng_fd_plugin_readpipe(service, p, pi, pi->buffer +
		                                          old_content_offset);
	}

	/*if empty, dont waist memory */
	if (pi->buffer_len == 0 && pi->buffer) {
		D_("Freeing empty buffer..");
		free(pi->buffer);
		pi->buffer = NULL;
		pi->buffer_allocated = 0;
		pi->buffer_len = 0;
	}


	/*
	 * if EOF close pipes.
	 * Dont free buffer, until the whole process_h frees
	 */
	if (read_ret == 0) {
		D_("Closing fifos for %s.\n", service->name);
		if (pi->pipe[0] > 0)
			close(pi->pipe[0]);
		if (pi->pipe[1] > 0)
			close(pi->pipe[1]);
		pi->pipe[0] = -1;
		pi->pipe[1] = -1;

		/* else, realloc to exact size */
		if (pi->buffer && pi->buffer_allocated > (pi->buffer_len + 1)) {
			tmp = initng_toolbox_realloc(pi->buffer,
					(pi->buffer_len + 1) * sizeof(char));
			if (tmp) {
				pi->buffer = tmp;
				pi->buffer_allocated = pi->buffer_len;
			}
		}
		return;
	}

	/* if buffer reached 10000 chars */
	if (pi->buffer_len > 10000) {
		D_("Buffer to big (%i > 10000), purging...", pi->buffer_len);
		/* copy the last 9000 chars to start */
		memmove(pi->buffer, &pi->buffer[pi->buffer_len - 9000], 9000);
		/* rezise the buffer - leave some expansion space! */
		tmp = initng_toolbox_realloc(pi->buffer, 9501);

		/* make sure realloc suceeded */
		if (tmp) {
			pi->buffer = tmp;
			pi->buffer_allocated = 9500;
		} else {
			F_("realloc failed, possibly out of memory!\n");
		}

		/* Even if realloc failed, the buffer is still valid
		   and we've still reduced the length of its contents */
		pi->buffer_len = 9000;		/* shortened by 1000 chars */
		pi->buffer[9000] = '\0';	/* shortened by 1000 chars */
	}

	D_("function done...");
}
