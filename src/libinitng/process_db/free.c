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

#include <initng.h>

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <alloca.h>
#include <assert.h>


void initng_process_db_clear_freed(active_db_h * service)
{
	process_h *current, *safe = NULL;

	while_processes_safe(current, safe, service)
	{
		if (current->pst != P_FREE)
			continue;

		initng_process_db_real_free(current);
	}
}

/* function to free a process_h struct */
void initng_process_db_real_free(process_h * free_this)
{
	pipe_h *current_pipe = NULL;
	pipe_h *current_pipe_safe = NULL;

	assert(free_this);

	/* Make sure this entry are not on any list */
	list_del(&free_this->list);

	while_pipes_safe(current_pipe, free_this, current_pipe_safe)
	{
		/* unbound this pipe from list */
		list_del(&current_pipe->list);

		/* close all pipes */
		if (current_pipe->pipe[0] > 0)
			close(current_pipe->pipe[0]);
		if (current_pipe->pipe[1] > 0)
			close(current_pipe->pipe[1]);

		/* free buffer */
		if (current_pipe->buffer)
			free(current_pipe->buffer);

		/* free it */
		free(current_pipe);
	}

	free(free_this);
	return;
}
