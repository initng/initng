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
	{
		s_event event;
		s_event_fd_watcher_data data;

		event.event_type = &EVENT_FD_WATCHER;
		event.data = &data;
		data.action = FDW_ACTION_CHECK;
		data.added = 0;
		data.readset = &readset;
		data.writeset = &writeset;
		data.errset = &errset;

		initng_event_send(&event);

		added += data.added;
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
	{
		s_event event;
		s_event_fd_watcher_data data;

		event.event_type = &EVENT_FD_WATCHER;
		event.data = &data;
		data.action = FDW_ACTION_CALL;
		data.readset = &readset;
		data.writeset = &writeset;
		data.errset = &errset;
		data.added = retval;

		initng_event_send(&event);

		retval = data.added;
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
