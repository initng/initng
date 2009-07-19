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
 * Gets an special process from an service
 * if it exists
 */
process_h *initng_process_db_get(ptype_h * type, active_db_h * service)
{
	process_h *current = NULL;

	while_processes(current, service) {
		if (current->pst != P_ACTIVE)
			continue;
		if (current->pt == type)
			return current;
	}

	return NULL;
}

/*
 * Gets an special process from an service
 * if it exists
 */
process_h *initng_process_db_get_by_name(const char *name,
					 active_db_h * service)
{
	process_h *current = NULL;

	while_processes(current, service) {
		if (current->pst != P_ACTIVE)
			continue;
		if (strcmp(current->pt->name, name) == 0)
			return current;
	}

	return NULL;
}

/*
 * Finds a process by its pid.
 */
process_h *initng_process_db_get_by_pid(pid_t pid, active_db_h * service)
{
	process_h *current = NULL;

	while_processes(current, service) {
		if (current->pst != P_ACTIVE)
			continue;
		if (current->pid == pid)
			return current;
	}

	return NULL;
}
