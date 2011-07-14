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

#include <sys/time.h>
#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <string.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* free() exit() */
#include <assert.h>

#include <initng.h>

/*
 * Use this function to change the status of an service, this
 * function might refuse to change to that state, and if that
 * it will return FALSE, please always check the return value
 * when calling this function.
 */
int initng_common_mark_service(active_db_h * service, a_state_h * state)
{
	assert(service);
	assert(service->name);
	assert(service->current_state);
	assert(state);

	if (service->state_lock && service->next_state) {
		W_("Trying to set state %s on locked service %s!\n",
		   state->name, service->name);
		return FALSE;
	}

	/* Test if already set */
	if (service->current_state == state) {
		D_("state %s is already set on %s!\n",
		   state->name, service->name);

		/* clear next_state */
		service->next_state = NULL;
		return TRUE;
	}

	D_("Going to mark service %s from %s to %s\n", service->name,
	   service->current_state->name, state->name);

	service->next_state = state;

	/* If state is not locked, update it */
	if (!service->state_lock)
		initng_common_state_unlock(service);

	return TRUE;
}

void initng_common_state_lock(active_db_h * service)
{
	assert(service);
	assert(service->name);

	service->state_lock++;
	D_("Locked state of %s (level: %d)\n", service->name,
	   service->state_lock);
}

int initng_common_state_unlock(active_db_h * service)
{
	assert(service);
	assert(service->name);
	assert(service->current_state);

	/* If locked, decrement the lock counter */
	if (service->state_lock) {
		/* Test if locked more than one time */
		if (--service->state_lock) {
			D_("State of %s is still locked (level: %d)\n",
			   service->name, service->state_lock);
			return FALSE;
		}
	}

	/* Test if state has changed */
	if (!service->next_state)
		return FALSE;

	D_(" %-20s : %10s -> %-10s\n", service->name,
	   service->current_state->name, service->next_state->name);

	/* Fill last entries */
	service->last_state = service->current_state;
	memcpy(&service->time_last_state, &service->time_current_state,
	       sizeof(struct timeval));

	/* update rough last to */
	if (service->last_rought_state != service->current_state->is) {
		service->last_rought_state = service->current_state->is;
		memcpy(&service->last_rought_time,
		       &service->time_current_state, sizeof(memcpy));
	}

	/* reset alarm, set state and time */
	service->alarm = 0;
	service->current_state = service->next_state;
	gettimeofday(&service->time_current_state, NULL);

	/* Set INTERRUPT, the interrupt is set only when a service
	 * changes state, and all state handlers will be called
	 */
	initng_list_add(&service->interrupt, &g.active_db.interrupt);

	/* clear next_state */
	service->next_state = NULL;

	D_("Unlocked state of %s\n", service->name);

	return TRUE;
}

void initng_common_state_lock_all(void)
{
	active_db_h *current;

	while_active_db(current) {
		initng_common_state_lock(current);
	}
}

int initng_common_state_unlock_all(void)
{
	active_db_h *current;
	int ret = 0;

	while_active_db(current) {
		ret |= initng_common_state_unlock(current);
	}

	return ret;
}

a_state_h *initng_common_state_has_changed(active_db_h * service)
{
	return service->next_state;
}
