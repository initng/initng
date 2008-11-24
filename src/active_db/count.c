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
#include <alloca.h>
#include <assert.h>
#include <fnmatch.h>
#include <sys/time.h>
#include <time.h>

#include <initng.h>

/**
 * Count services on a given state.
 *
 * @param state
 * @return number of services
 *
 * Counts the number of services currently on a given state, or
 * if the parameter is NULL, counts all services.
 */
int initng_active_db_count(a_state_h * current_state_to_count)
{
	int counter = 0;	/* actives counter */
	active_db_h *current = NULL;

	/* ok, go COUNT ALL */
	if (!current_state_to_count) {
		/* ok, go through all */
		while_active_db(current) {
			assert(current->name);

			/* count almost all */

			/* but not failed services */
			if (IS_FAILED(current))
				continue;
			/* and not stopped */
			if (IS_DOWN(current))
				continue;

			counter++;
		}

		return counter;
	}

	/* ok, go COUNT A SPECIAL */
	while_active_db(current) {
		assert(current->name);
		/* check if this is the status to count */
		if (current->current_state == current_state_to_count)
			counter++;
	}
	/* return counter */
	return counter;
}
