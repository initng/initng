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
#include "initng.h"
#include "initng_global.h"
#include "initng_service_cache.h"
#include "initng_load_module.h"
#include "initng_toolbox.h"
#include "initng_signal.h"

#include "initng_plugin_callers.h"
#include "initng_plugin.h"

#include "initng_fd.h"

void initng_fd_close_all(void)
{
	s_call *current, *safe = NULL;

	S_;

	while_list_safe(current, &g.FDWATCHERS, safe)
	{
		if (current->c.fdh->fds > 0)
			close(current->c.fdh->fds);
		list_del(&current->list);

		if (current->from_file)
			free(current->from_file);
		free(current);
	}
}

/*
 * This function delivers what read to plugin,
 * through the hooks.
 * If no hook is found, or no return TRUE, it will
 * be printed to screen anyway.
 */
static void initng_fd_plugin_readpipe(active_db_h * service,
									  process_h * process, pipe_h * pi,
									  char *buffer_pos)
{
	s_call *current = NULL;
	int delivered = FALSE;

	S_;


	while_list(current, &g.BUFFER_WATCHER)
	{
		D_("Calling pipewatcher plugin.\n");
		if ((*current->c.buffer_watcher) (service, process, pi,
										  buffer_pos) == TRUE)
			delivered = TRUE;
#ifdef DEBUG
		else
		{
			D_("plugin %s returned FALSE\n", current->from_file);
		}
#endif
	}

	/* make sure someone handled this */
	if (delivered != TRUE)
		fprintf(stdout, "%s", buffer_pos);
}


/* if there is data incomming in a pipe, tell the plugins */
static int initng_fd_pipe(active_db_h * service, process_h * process,
						  pipe_h * pi)
{
	s_call *current = NULL;

	while_list(current, &g.PIPE_WATCHER)
	{
		if ((*current->c.pipe_watcher) (service, process, pi) == TRUE)
			return (TRUE);
	}

	return (FALSE);
}


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

	D_("\ninitng_fd_process_read_input(%s, %s, %i);\n", service->name,
	   p->pt->name);

	if (pi->pipe[0] <= 0)
	{
		F_("FIFO, can't be read! NOT OPEN!\n");
		return;
	}

	/* INITZIATE
	 * if this are not set to out_pipe, if there is nothing to read. read() will block.
	 * initng and sit down waiting for input.
	 */
	if (!pi->buffer)
	{
		/* initziate buffer fnctl */
		int fd_flags;

		fd_flags = fcntl(pi->pipe[0], F_GETFL, 0);
		fcntl(pi->pipe[0], F_SETFL, fd_flags | O_NONBLOCK);
	}

	/* read data from process, and continue again after a interrupt */
	do
	{
		errno = 0;

		/* OBSERVE, i_realloc may change the path to the data, so dont set buffer_pos to early */

		/* Make sure there is room for 100 more chars */
		D_("left: %i > %i\n", pi->buffer_len + 100, pi->buffer_allocated);
		if (pi->buffer_len + 100 >= pi->buffer_allocated)
		{
			/* do a realloc */
			D_("Changing size of buffer %p to: %i\n", pi->buffer,
			   pi->buffer_allocated + 100 + 1);
			tmp = i_realloc(pi->buffer,
							(pi->buffer_allocated + 100 + 1) * sizeof(char));

			/* make sure realloc suceeded */
			if (tmp)
			{
				D_("pi->buffer changes from %p to %p.\n", pi->buffer, tmp);
				pi->buffer = tmp;
				pi->buffer_allocated += 100;

				/*
				 * make sure it nulls, specially when i_realloc is run for the verry first time
				 * and maby there is nothing to get by read
				 */
				pi->buffer[pi->buffer_len] = '\0';
			}
			else
			{
				F_("realloc failed, possibly out of memory!\n");
				return;
			}
		}

		/* read the data */
		D_("Reading 100 chars.\n");
		read_ret = read(pi->pipe[0], &pi->buffer[pi->buffer_len], 100);
		/*printf("read_ret = %i  : \"%.100s\"\n", read_ret, read_pos); */

		/* make sure read does not return -1 */
		if (read_ret <= 0)
			break;

		/* increase buffer_len */
		pi->buffer_len += read_ret;

		/* make sure its nulled at end */
		pi->buffer[pi->buffer_len] = '\0';
	}
	/* if read_ret == 100, it migit be more to read, or it got interrupted. */
	while (read_ret >= 100 || errno == EINTR);



	/* make sure there is any */
	if (pi->buffer_len > old_content_offset)
	{
		/* let all plugin take part of data */
		initng_fd_plugin_readpipe(service, p, pi,
								  pi->buffer + old_content_offset);
	}

	/*if empty, dont waist memory */
	if (pi->buffer_len == 0 && pi->buffer)
	{
		free(pi->buffer);
		pi->buffer = NULL;
		pi->buffer_allocated = 0;
		pi->buffer_len = 0;
	}


	/*
	 * if EOF close pipes.
	 * Dont free buffer, until the whole process_h frees
	 */
	if (read_ret == 0)
	{
		D_("Closing fifos for %s.\n", service->name);
		if (pi->pipe[0] > 0)
			close(pi->pipe[0]);
		if (pi->pipe[1] > 0)
			close(pi->pipe[1]);
		pi->pipe[0] = -1;
		pi->pipe[1] = -1;

		/* else, realloc to exact size */
		if (pi->buffer && pi->buffer_allocated > (pi->buffer_len + 1))
		{
			tmp = i_realloc(pi->buffer, (pi->buffer_len + 1) * sizeof(char));
			if (tmp)
			{
				pi->buffer = tmp;
				pi->buffer_allocated = pi->buffer_len;
			}
		}
		return;
	}

	/* if buffer reached 10000 chars */
	if (pi->buffer_len > 10000)
	{
		/* copy the last 9000 chars to start */
		memmove(pi->buffer, &pi->buffer[pi->buffer_len - 9000],
				9000 * sizeof(char));
		/* rezise the buffer - leave some expansion space! */
		tmp = i_realloc(pi->buffer, 9501 * sizeof(char));

		/* make sure realloc suceeded */
		if (tmp)
		{
			pi->buffer = tmp;
			pi->buffer_allocated = 9500;
		}
		else
		{
			F_("realloc failed, possibly out of memory!\n");
		}

		/* Even if realloc failed, the buffer is still valid
		   and we've still reduced the length of its contents */
		pi->buffer_len = 9000;				/* shortened by 1000 chars */
		pi->buffer[9000] = '\0';			/* shortened by 1000 chars */
	}
}

#define STILL_OPEN(fd) (fcntl(fd, F_GETFD)>=0)

/*
 * FILEDESCRIPTORPOLLNG
 *
 * File descriptors are scanned in 3 steps.
 * First, scan all plug-ins, and processes after file descriptors to
 * watch, and add them with FD_SET(); added++;
 *
 * Second, Run select if there where any added, with an timeout set
 * when function was called.
 *
 * Third, if select returns that there is data to fetch, walk over
 * plug-ins and file descriptors again, and call plug-ins or initng_process_readpipe
 * with pointer to plugin/service.
 *
 * This function will return FALSE, if it timeout, or if there was
 * no fds to monitor.
 */
void initng_fd_plugin_poll(int timeout)
{
	/* Variables */
	fd_set readset, writeset, errset;
	struct timeval tv;
	int retval;
	int added = 0;
	active_db_h *currentA, *qA;
	s_call *currentC, *qC;
	process_h *currentP, *qP;
	pipe_h *current_pipe;

	/* initialization */
	S_;
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_ZERO(&errset);

	/* Set timeval struct, to our timeout */
	tv.tv_sec = timeout;
	tv.tv_usec = 0;


	/*
	 * STEP 1:  Scan for fds to add
	 */


	/* scan through active plug-ins that have listening file descriptors, and add them */
	currentC = NULL;
	while_list(currentC, &g.FDWATCHERS)
	{
		/* first, some checking */
		if (currentC->c.fdh->fds <= 2)
			continue;
		if (!currentC->c.fdh->call_module)
			continue;

		/* This is a expensive test, but better safe then sorry */
		if (!STILL_OPEN(currentC->c.fdh->fds))
		{
			D_("%i is not open anymore.\n", currentC->c.fdh->fds);
			currentC->c.fdh->fds = -1;
			continue;
		}

		/*D_("adding fd #%i, from an call_db.\n", current->c.fdh->fds); */
		if (currentC->c.fdh->what & FDW_READ)
			FD_SET(currentC->c.fdh->fds, &readset);
		if (currentC->c.fdh->what & FDW_WRITE)
			FD_SET(currentC->c.fdh->fds, &writeset);
		if (currentC->c.fdh->what & FDW_ERROR)
			FD_SET(currentC->c.fdh->fds, &errset);
		added++;
	}

	/* Then add all file descriptors from process read pipes */
	currentA = NULL;
	while_active_db(currentA)
	{
		currentP = NULL;
		while_processes(currentP, currentA)
		{
			current_pipe = NULL;
			while_pipes(current_pipe, currentP)
			{
				if ((current_pipe->dir == OUT_PIPE ||
					 current_pipe->dir == BUFFERED_OUT_PIPE) &&
					current_pipe->pipe[0] > 2)
				{
					/* expensive test to make sure the pipe is open, before adding */
					if (!STILL_OPEN(current_pipe->pipe[0]))
					{
						D_("%i is not open anymore.\n",
						   current_pipe->pipe[0]);
						current_pipe->pipe[0] = -1;
						continue;
					}


					FD_SET(current_pipe->pipe[0], &readset);
					added++;
				}

				if (current_pipe->dir == IN_AND_OUT_PIPE &&
					current_pipe->pipe[1] > 2)
				{
					/* expensive test to make sure the pipe is open, before adding */
					if (!STILL_OPEN(current_pipe->pipe[1]))
					{
						D_("%i is not open anymore.\n",
						   current_pipe->pipe[1]);
						current_pipe->pipe[1] = -1;
						continue;
					}


					FD_SET(current_pipe->pipe[1], &readset);
					added++;
				}

				if (current_pipe->dir == IN_PIPE && current_pipe->pipe[1] > 2)
				{
					/* expensive test to make sure the pipe is open, before adding */
					if (!STILL_OPEN(current_pipe->pipe[1]))
					{
						D_("%i is not open anymore.\n",
						   current_pipe->pipe[1]);
						current_pipe->pipe[1] = -1;
						continue;
					}


					FD_SET(current_pipe->pipe[1], &writeset);
					added++;
				}
			}
		}
	}


	/*
	 * STEP 2: Do the select-poll, if any fds where added
	 */

	/* check if there are any set */
	if (added <= 0)
	{
		D_("No file descriptors set.\n");
		sleep(timeout);
		return;
	}
	D_("%i file descriptors added.\n", added);


	/* make the select */
	retval = select(256, &readset, &writeset, &errset, &tv);


	/* error - Truly a interrupt */
	if (retval < 0)
	{
		D_("Select returned %i\n", retval);
		return;
	}

	/* none was found */
	if (retval == 0)
	{
		D_("There was no data found on any added fd.\n");
		return;
	}

	D_("%d fd's active\n", retval);

	/* If a fsck is running select will always return one file handler active, we give it 0.1 seconds to get some more data into the buffer. */
	sleep(0.1);


	/*
	 * STEP 3:  Find the plug-ins/processes that handles the fd input, and call them
	 */

	/* Now scan through callers */
	currentC = NULL;
	qC = NULL;
	while_list_safe(currentC, &g.FDWATCHERS, qC)
	{
		int what = 0;

		if (currentC->c.fdh->fds <= 2)
			continue;
		if (!currentC->c.fdh->call_module)
			continue;

		if (FD_ISSET(currentC->c.fdh->fds, &readset))
			what |= FDW_READ;
		if (FD_ISSET(currentC->c.fdh->fds, &writeset))
			what |= FDW_WRITE;
		if (FD_ISSET(currentC->c.fdh->fds, &errset))
			what |= FDW_ERROR;

		if (what == 0)
			continue;


		D_("Calling plugin handler for fd #%i, what=%i\n",
		   currentC->c.fdh->fds, what);
		(*currentC->c.fdh->call_module) (currentC->c.fdh, what);
		D_("Call handler returned.\n");

		/* Found match, that means we need to look for one less, if we've found all we should then return */
		retval--;
		if (retval == 0)
			return;
	}

	/* scan every service */
	currentA = NULL;
	qA = NULL;
	while_active_db_safe(currentA, qA)
	{
		currentP = NULL;
		qP = NULL;
		/* and all the processes */
		while_processes_safe(currentP, qP, currentA)
		{
			current_pipe = NULL;

			/* check if this fd is a pipe bound to a process */
			while_pipes(current_pipe, currentP)
			{
				/* if a bufered output pipe is found . */
				if (current_pipe->dir == BUFFERED_OUT_PIPE
					&& current_pipe->pipe[0] > 2
					&& FD_ISSET(current_pipe->pipe[0], &readset))
				{
					D_("BUFFERED_OUT_PIPE: Will read from %s->start_process on fd #%i\n", currentA->name, current_pipe->pipe[0]);

					/* Do the actual read from pipe */
					initng_fd_process_read_input(currentA, currentP,
												 current_pipe);

					/* Found match, that means we need to look for one less, if we've found all we should then return */
					retval--;
					if (retval == 0)
						return;
				}

				if (current_pipe->dir == OUT_PIPE &&
					current_pipe->pipe[0] > 2 &&
					FD_ISSET(current_pipe->pipe[0], &readset))
				{
					initng_fd_pipe(currentA, currentP, current_pipe);

					/* Found match, that means we need to look for one less, if we've found all we should then return */
					retval--;
					if (retval == 0)
						return;
				}

				if (current_pipe->dir == IN_AND_OUT_PIPE &&
					current_pipe->pipe[1] > 2 &&
					FD_ISSET(current_pipe->pipe[1], &readset))
				{
					initng_fd_pipe(currentA, currentP, current_pipe);

					/* Found match, that means we need to look for one less, if we've found all we should then return */
					retval--;
					if (retval == 0)
						return;
				}

				if (current_pipe->dir == IN_PIPE &&
					current_pipe->pipe[1] > 2 &&
					FD_ISSET(current_pipe->pipe[1], &writeset))
				{
					initng_fd_pipe(currentA, currentP, current_pipe);

					/* Found match, that means we need to look for one less, if we've found all we should then return */
					retval--;
					if (retval == 0)
						return;
				}

			}
		}
	}

	if (retval != 0)
	{
		F_("There is a missed pipe or fd that we missed to poll!\n");
	}

	return;
}
