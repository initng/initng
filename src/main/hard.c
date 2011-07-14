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

#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <string.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdlib.h>		/* free() exit() */
#ifndef __HAIKU__
#include <sys/reboot.h>		/* reboot() RB_DISABLE_CAD */
#include <sys/mount.h>
#endif
#include <termios.h>
#include <stdio.h>
#include <errno.h>

#include "local.h"

void hard(h_then t)
{
#ifdef CHECK_RO
	FILE *test;
#endif
	int pid;

	/* set the sys state */
	switch (t) {
	case THEN_REBOOT:
		initng_main_set_sys_state(STATE_REBOOT);
		break;

	case THEN_HALT:
		initng_main_set_sys_state(STATE_HALT);
		break;

	case THEN_POWEROFF:
		initng_main_set_sys_state(STATE_POWEROFF);
		break;
	default:
		F_("initng_hard can only handle STATE_REBOOT, STATE_POWEROFF,"
		   " or STATE_HALT for now.\n");
		return;
	}

	/* sync data to disk */
	sync();

	/* make sure we are root */
	if (getuid() != 0)
		return;

	/* unload all modules found */
	initng_module_unload_all();

	/* TODO: should we make sure all fds are closed at this point? */

	/* last sync data to disk */
	sync();

#ifdef CHECK_RO
	/* Mount readonly, youst to be extra sure this is done */
	mount("/dev/root", "/", NULL, MS_RDONLY + MS_REMOUNT, NULL);

	if (errno == EBUSY) {
		F_("Failed to remount / ro, EBUSY\n");
	}

	/* check so that / is mounted read only, by trying to open a file */
	test = fopen("/initng_write_testfile", "w");
	if (test) {
		fclose(test);
		unlink("/initng_write_testfile");
		F_("/ IS NOT REMOUNTED READ-ONLY, WON'T REBOOT/HALT BECAUSE THE FILE SYSTEM CAN BREAK!\n");
		return;
	}
#endif

	/* Under certain unknown circumstances, calling reboot(RB_POWER_OFF)
	 * from pid 1 leads to a "Kernel panic - not syncing: Attempted to
	 * kill init!". A workaround is to fork a child to do it. See bug #488
	 * for details */
	pid = fork();

	/* if succeded (pid==0) or failed (pid < 0) */
	if (pid <= 0) {
#ifndef __HAIKU__
		/* child process - shut down the machine */
		switch (t) {
		case THEN_REBOOT:
			reboot(RB_AUTOBOOT);
			break;

		case THEN_HALT:
			reboot(RB_HALT_SYSTEM);
			break;

		case THEN_POWEROFF:
			reboot(RB_POWER_OFF);
			break;

		default:
			/* What to do here */
			break;
		}
#endif

		/* if in fork, quit it */
		if (pid == 0)
			_exit(0);
	}

	/* parent process waits for child to exit */
	if (pid > 0)
		waitpid(pid, NULL, 0);

	/* idle forever */
	while (1)
		sleep(100);
}
