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

#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <sys/un.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <linux/kd.h>		/* KDSIGACCEPT */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdlib.h>		/* free() exit() */
#include <sys/reboot.h>		/* reboot() RB_DISABLE_CAD */
#include <sys/mount.h>
#include <termios.h>
#include <stdio.h>
#include <sys/klog.h>
#include <errno.h>

#include "local.h"

/* this is called when there is no processes left */
void initng_main_when_out(void)
{

	int failing = 0;
	active_db_h *current = NULL;

	while_active_db(current) {
		if (IS_FAILED(current)) {
			failing++;
			printf("\n [%i] service \"%s\" marked \"%s\"\n",
			       failing, current->name,
			       current->current_state->name);
		}
	}

	if (failing > 0) {
		printf("\n\n All %i services listed above, are marked with a "
		       "failure.\n"
		       " Will sleep for 15 seconds before reboot/halt so you "
		       "can see them.\n\n", failing);
		sleep(15);
	}

	if (g.i_am == I_AM_INIT && getpid() != 1) {
		F_("I AM NOT INIT, THIS CANT BE HAPPENING!\n");
		sleep(3);
		return;
	}

	/* always good to do */
	sync();

	/* none of these calls should return, so the su_login on the end will be a fallback */
	switch (g.when_out) {
	case THEN_QUIT:
		P_(" ** Now Quiting **\n");
		initng_main_exit(0);
		break;

	case THEN_SULOGIN:
		P_(" ** Now SuLogin\n");
		/* break here leads to su_login below */
		break;

	case THEN_RESTART:
		P_(" ** Now restarting\n");
		initng_main_restart();
		break;

	case THEN_NEW_INIT:
		P_(" ** Launching new init\n");
		initng_main_new_init();
		break;

	case THEN_REBOOT:
	case THEN_HALT:
	case THEN_POWEROFF:
		hard(g.when_out);
		break;
	}

	/* fallback */
	initng_main_su_login();
}
