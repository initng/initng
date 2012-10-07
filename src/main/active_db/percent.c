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
 * Calculate the percentage of started processes.
 *
 * @return percentage
 *
 * Walks throught the active_db looking for started processes and calculates
 * the percentage of started ones.
 */
int initng_active_db_percent_started(void)
{
	active_db_h *current = NULL;
	int starting = 0, up = 0, other = 0;

	while_active_db(current) {
		assert(current->name);
		assert(current->current_state);

		switch (GET_STATE(current)) {
		case IS_STARTING:
			starting++;
			break;
		case IS_UP:
			up++;
			break;
		default:
			other++;
			break;
		}
	}
	D_("up: %i  starting: %i  other: %i\n", up, starting, other);

	int ret = 0;
	/* if no one is starting */
	if (starting <= 0)
		ret = 100;
	else if (up > 0) {
		ret = 100 * up / (starting + up);
		D_("up/starting: %i  percent: %i\n", up / starting, ret);
	}
	return ret;
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
	active_db_h *current = NULL;
	int stopping = 0, down = 0, other = 0;

	while_active_db(current) {
		assert(current->name);
		assert(current->current_state);

		switch (GET_STATE(current)) {
		case IS_DOWN:
			down++;
			break;
		case IS_STOPPING:
			stopping++;
			break;
		default:
			other++;
			break;
		}
	}

	D_("down: %i  stopping: %i  other: %i\n", down, stopping, other);

	int ret = 0;
	/* if no one stopping */
	if (stopping <= 0)
		ret = 100;
	else if (down > 0) {
		ret = 100 * down / (stopping + down);
		D_("down/stopping: %f  percent: %i\n", (double)(down / stopping), ret);
	}
	return ret;
}
