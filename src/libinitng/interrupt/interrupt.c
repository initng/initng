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

/* common standard first define */
#include "initng.h"

/* system headers */
#include <sys/time.h>
#include <time.h>							/* time() */
#include <fcntl.h>							/* fcntl() */
#include <sys/un.h>							/* memmove() strcmp() */
#include <sys/wait.h>						/* waitpid() sa */
#include <linux/kd.h>						/* KDSIGACCEPT */
#include <sys/ioctl.h>						/* ioctl() */
#include <stdio.h>							/* printf() */
#include <stdlib.h>							/* free() exit() */
#include <sys/reboot.h>						/* reboot() RB_DISABLE_CAD */
#include <assert.h>

#include "initng_common.h"

/* local headers include */
#include "initng_main.h"
#include "initng_active_db.h"
#include "initng_load_module.h"
#include "initng_plugin_callers.h"
#include "initng_toolbox.h"
#include "initng_string_tools.h"
#include "initng_global.h"
#include "initng_static_states.h"
#include "initng_depend.h"
#include "initng_handler.h"
#include "initng_static_event_types.h"

/* self */
#include "initng_interrupt.h"


int initng_interrupt(void)
{
	active_db_h *service, *safe = NULL;
	int interrupt = FALSE;

	/*D_(" ** INTERRUPT **\n"); */

	/* check what services that changed state */
	while_active_db_interrupt_safe(service, safe)
	{
		/* mark as interrupt run */
		interrupt = TRUE;

		/* remove from interrupt list */
		list_del(&service->interrupt);

		/* handle this one. */
		handle(service);
	}

	/* if there was any interupt, run interupt handler hooks */
	if (interrupt)
		initng_handler_run_interrupt_handlers();

	/* return positive if any interupt was handled */
	return (interrupt);
}
