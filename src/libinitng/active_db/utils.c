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

#include "initng.h"

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <assert.h>
#include <fnmatch.h>
#include <sys/time.h>
#include <time.h>

#include "initng_global.h"
#include "initng_active_db.h"
#include "initng_process_db.h"
#include "initng_toolbox.h"
#include "initng_common.h"
#include "initng_static_data_id.h"
#include "initng_plugin_callers.h"
#include "initng_string_tools.h"
#include "initng_static_states.h"
#include "initng_depend.h"


/* compensate time */
void initng_active_db_compensate_time(time_t skew)
{
	active_db_h *current = NULL;

	/* walk the active_db */
	while_active_db(current)
	{
		assert(current->name);
		/* change this time */
		current->time_current_state.tv_sec += skew;
		current->time_last_state.tv_sec += skew;
		current->last_rought_time.tv_sec += skew;
		current->alarm += skew;
	}
}

/* active_db_count counts a type, if null, count all */
int initng_active_db_count(a_state_h * current_state_to_count)
{
	int counter = 0;			/* actives counter */
	active_db_h *current = NULL;

	/* ok, go COUNT ALL */
	if (!current_state_to_count)
	{
		/* ok, go through all */
		while_active_db(current)
		{
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

		return (counter);
	}

	/* ok, go COUNT A SPECIAL */
	while_active_db(current)
	{
		assert(current->name);
		/* check if this is the status to count */
		if (current->current_state == current_state_to_count)
			counter++;
	}
	/* return counter */
	return (counter);
}


/* calculate percent of processes started */
int initng_active_db_percent_started(void)
{
	int starting = 0;
	int up = 0;
	int other = 0;
	float tmp = 0;

	active_db_h *current = NULL;

	/* walk the active_db */
	while_active_db(current)
	{
		assert(current->name);
		assert(current->current_state);

		/* count starting */
		if (IS_STARTING(current))
		{
			starting++;
			continue;
		}

		/* count up */
		if (IS_UP(current))
		{
			up++;
			continue;
		}

		/* count others */
		other++;
	}
	D_("active_db_percent_started(): up: %i   starting: %i  other: %i\n", up,
	   starting, other);

	/* if no one starting */
	if (starting <= 0)
		return (100);

	if (up > 0)
	{
		tmp = 100 * (float) up / (float) (starting + up);
		D_("active_db_percent_started(): up/starting: %f percent: %i\n\n",
		   (float) up / (float) starting, (int) tmp);
		return ((int) tmp);
	}
	return 0;
}

/* calculate percent of processes stopped */
int initng_active_db_percent_stopped(void)
{
	int stopping = 0;
	int down = 0;
	int other = 0;
	float tmp = 0;
	active_db_h *current = NULL;

	while_active_db(current)
	{
		assert(current->name);
		assert(current->current_state);

		/* count stopped services */
		if (IS_DOWN(current))
		{
			down++;
			continue;
		}

		/* count stopping */
		if (IS_STOPPING(current))
		{
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

	if (down > 0)
	{
		tmp = 100 * (float) down / (float) (stopping + down);
		D_("active_db_percent_stopped(): down/stopping: %f percent: %i\n\n",
		   (float) down / (float) stopping, (int) tmp);
		return ((int) tmp);
	}
	return 0;
}



/*
 * Walk the active db, searching for services that are down, and been so for a minute.
 * It will remove this entry to save memory.
 * CLEAN_DELAY are defined in initng.h
 */
void initng_active_db_clean_down(void)
{
	active_db_h *current = NULL;
	active_db_h *safe = NULL;

	while_active_db_safe(current, safe)
	{
		assert(current->name);
		assert(current->current_state);
		if (g.now.tv_sec > current->time_current_state.tv_sec + CLEAN_DELAY)
		{
			initng_process_db_clear_freed(current);

			/* count stopped services */
			if (!IS_DOWN(current))
				continue;

			initng_active_db_free(current);
		}
	}
}
