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
 * Calculate the percentage of started processes.
 *
 * @return percentage
 *
 * Walks throught the active_db looking for started processes and calculates
 * the percentage of started ones.
 */
int initng_active_db_percent_started(void)
{
	int starting = 0;
	int up = 0;
	int other = 0;
	float tmp = 0;

	active_db_h *current = NULL;

	/* walk the active_db */
	while_active_db(current) {
		assert(current->name);
		assert(current->current_state);

		/* count starting */
		if (IS_STARTING(current)) {
			starting++;
			continue;
		}

		/* count up */
		if (IS_UP(current)) {
			up++;
			continue;
		}

		/* count others */
		other++;
	}
	D_("active_db_percent_started(): up: %i   starting: %i  other: %i\n",
	   up, starting, other);

	/* if no one starting */
	if (starting <= 0)
		return (100);

	if (up > 0) {
		tmp = 100 * (float)up / (float)(starting + up);
		D_("active_db_percent_started(): up/starting: %f percent: %i\n\n", (float)up / (float)starting, (int)tmp);
		return ((int)tmp);
	}
	return 0;
}

/**
 * Calculate percentage of stopped processes.
 *
 * @return percentage
 *
 * Walks throught the active_db looking for stopped processes and calculates
 * the percentage of started ones.
 */
int initng_active_db_percent_stopped(void)
{
	int stopping = 0;
	int down = 0;
	int other = 0;
	float tmp = 0;
	active_db_h *current = NULL;

	while_active_db(current) {
		assert(current->name);
		assert(current->current_state);

		/* count stopped services */
		if (IS_DOWN(current)) {
			down++;
			continue;
		}

		/* count stopping */
		if (IS_STOPPING(current)) {
			stopping++;
			continue;
		}

		/* count others */
		other++;
	}

	D_("active_db_percent_stopped(): down: %i   stopping: %i  other: %i\n",
	   down, stopping, other);

	/* if no one stopping */
	if (stopping <= 0)
		return (100);

	if (down > 0) {
		tmp = 100 * (float)down / (float)(stopping + down);
		D_("active_db_percent_stopped(): "
		   "down/stopping: %f percent: %i\n\n",
		   (float)down / (float)stopping, (int)tmp);
		return (int)tmp;
	}
	return 0;
}
