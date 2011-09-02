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
 * Compensate time.
 *
 * @param adjustment
 *
 * Walks the active_db, adjusting any service's time-type variables.
 */
void initng_active_db_compensate_time(time_t skew)
{
	active_db_h *current = NULL;

	/* walk the active_db */
	while_active_db(current) {
		assert(current->name);
		/* change this time */
		current->time_current_state.tv_sec += skew;
		current->time_last_state.tv_sec += skew;
		current->last_rought_time.tv_sec += skew;
		current->alarm += skew;
	}
}
