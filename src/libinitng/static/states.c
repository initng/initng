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

#include <initng.h>

#include <time.h>				/* time() */
#include <fcntl.h>				/* fcntl() */
#include <sys/un.h>				/* memmove() strcmp() */
#include <sys/wait.h>				/* waitpid() sa */
#include <linux/kd.h>				/* KDSIGACCEPT */
#include <sys/ioctl.h>				/* ioctl() */
#include <stdio.h>				/* printf() */
#include <stdlib.h>				/* free() exit() */
#include <sys/reboot.h>				/* reboot() RB_DISABLE_CAD */
#include <assert.h>
#include <errno.h>


/*
 * a_state_h
 *
 * const char *NAME
 * int is_rought
 * void state_interrupt
 */

a_state_h NEW = {
	.name = "NEW",
	.description = "This is a newly created service, that has not got a "
	               "real state yet.",
	.is = IS_DOWN,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h LOADING = {
	.name = "LOADING",
	.description = "This service is loading service data from disk.",
	.is = IS_DOWN,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h FREEING = {
	.name = "FREEING",
	.description = "This service is freeing, and will be removed soon.",
	.is = IS_DOWN,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

a_state_h REQ_NOT_FOUND = {
	.name = "REQ_NOT_FOUND",
	.description = "A Required service was not found, cant start.",
	.is = IS_FAILED,
	.interrupt = NULL,
	.init = NULL,
	.alarm = NULL
};

void initng_static_states_register_defaults(void)
{
	initng_active_state_register(&NEW);
	initng_active_state_register(&LOADING);
	initng_active_state_register(&FREEING);
	initng_active_state_register(&REQ_NOT_FOUND);
}
