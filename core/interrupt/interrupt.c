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
#include <string.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* free() exit() */
#include <assert.h>

#include "local.h"

int initng_interrupt(void)
{
	active_db_h *service, *safe = NULL;
	int interrupt = FALSE;

	/*D_(" ** INTERRUPT **\n"); */

	/* check what services that changed state */
	while_active_db_interrupt_safe(service, safe) {
		/* mark as interrupt run */
		interrupt = TRUE;

		/* remove from interrupt list */
		initng_list_del(&service->interrupt);

		/* handle this one. */
		handle(service);
	}

	/* if there was any interupt, run interupt handler hooks */
	if (interrupt)
		run_interrupt_handlers();

	/* return positive if any interupt was handled */
	return interrupt;
}
