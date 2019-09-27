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

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fnmatch.h>
#include <sys/time.h>
#include <time.h>

#include <initng.h>

/**
 * Creates a new service structure.
 *
 * @param service name
 * @return service structure
 */
active_db_h *initng_active_db_new(const char *name)
{
	active_db_h *new_active = NULL;

	assert(name);

	/* allocate a new active entry */
	new_active =
	    (active_db_h *) initng_toolbox_calloc(1, sizeof(active_db_h));
	if (!new_active) {
		F_("Unable to allocate active, out of memory?\n");
		return NULL;
	}

	/* initialize all lists */
	initng_list_init(&(new_active->processes.list));

	/* set the name */
	new_active->name = initng_toolbox_strdup(name);
	if (!new_active->name) {
		F_("Unable to set name, out of memory?\n");
		return NULL;
	}

	DATA_HEAD_INIT_REQUEST(&new_active->data, NULL, NULL);

	/* get the time, and copy that time to all time entries */
	initng_gettime_monotonic(&new_active->time_current_state);
	memcpy(&new_active->time_last_state, &new_active->time_current_state,
	       sizeof(struct timeval));
	memcpy(&new_active->last_rought_time, &new_active->time_current_state,
	       sizeof(struct timeval));

	/* mark this service as stopped, because it is not yet starting, or
	 * running */
	new_active->current_state = &NEW;

	/* reset alarm */
	new_active->alarm = 0;

	new_active->name_hash = initng_hash_str(name);

	/* return the newly created active_db_h */
	return new_active;
}
