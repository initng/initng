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

#include <sys/time.h>
#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <sys/un.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <linux/kd.h>		/* KDSIGACCEPT */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* free() exit() */
#include <sys/reboot.h>		/* reboot() RB_DISABLE_CAD */
#include <assert.h>
#include <errno.h>

/*
 * Find alarm, will brows active_db for states when the timeout have gone
 * out and run them.
 */
void initng_handler_run_alarm(void)
{
	active_db_h *current, *q = NULL;

	S_;

	/* reset next_alarm */
	g.next_alarm = 0;

	/* walk through all active services */
	while_active_db_safe(current, q) {
		assert(current->name);
		assert(current->current_state);

		/* skip services where alarm is not set */
		if (current->alarm == 0)
			continue;

		/* if alarm has gone */
		if (g.now.tv_sec >= current->alarm) {
			/* reset alarm */
			current->alarm = 0;

			/* call alarm handler, now when we got an g.interrupt */
			if (current->current_state->alarm)
				(*current->current_state->alarm) (current);

			continue;
		}

		/* if this alarm is more early than next_to_get, or
		   next_to_get isn't set. */
		if (current->alarm < g.next_alarm || !g.next_alarm)
			g.next_alarm = current->alarm;
	}
}
