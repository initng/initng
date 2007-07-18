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


/* creates a process_h struct with defaults */
process_h *initng_process_db_new(ptype_h * type)
{
	process_h *new_p = NULL;
	pipe_h *current_pipe;

	/* allocate a new process entry */
	new_p = (process_h *) initng_toolbox_calloc(1, sizeof(process_h));
	if (!new_p) {
		F_("Unable to allocate process!\n");
		return NULL;
	}

	new_p->pt = type;

	/* null pid */
	new_p->pid = 0;

	/* reset return code */
	new_p->r_code = 0;

	/* Set this to active, so it wont get freed */
	new_p->pst = P_ACTIVE;

	/* initziate list of pipes, so we can run add_pipe below without segfault */
	INIT_LIST_HEAD(&new_p->pipes.list);

	/* create the output pipe */
	current_pipe = initng_process_db_pipe_new(BUFFERED_OUT_PIPE);
	if (!current_pipe) {
		free(new_p);
		return NULL;
	}

	/* we want this pipe to get fd 1 and 2 in the fork */
	current_pipe->targets[0] = STDOUT_FILENO;
	current_pipe->targets[1] = STDERR_FILENO;
	add_pipe(current_pipe, new_p);

	/* return new process_h pointer */
	return new_p;
}
