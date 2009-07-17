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

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <alloca.h>
#include <assert.h>

/*
 * This function creates a new pipe, and creates a new
 * pipe struct entry.
 */
pipe_h *initng_process_db_pipe_new(e_dir dir)
{
	pipe_h *pipe_struct = initng_toolbox_calloc(1, sizeof(pipe_h));

	if (!pipe_struct)
		return NULL;

	/* set the type */
	pipe_struct->dir = dir;

	/* return the pointer */
	return pipe_struct;
}
