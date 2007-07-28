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

/*
 * Poll if there are any processes left, and if not quit initng
 */
int initng_main_ready_to_quit(void)
{
	active_db_h *current = NULL;

	/* If the last process has died, quit initng */
	while_active_db(current) {
		/* Don't check with failed services */
		if (IS_FAILED(current))
			continue;

		/* if its down, its nothing to count on */
		if (IS_DOWN(current))
			continue;

		/*
		 * if we got here,
		 * its must be an non failing service,
		 * left.
		 */
		return FALSE;
	}

	return TRUE;
}
