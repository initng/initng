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
#include <unistd.h>		/* execv() pipe() usleep() */
						/* pause() chown() pid_t   */
#include <sys/types.h>
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/ioctl.h>		/* ioctl() */
#include <sys/socket.h>		/* socketpair() */
#include <stdlib.h>		/* free() exit() */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pwd.h>
#include <sys/stat.h>
#include <errno.h>

void initng_fork_aforkhooks(active_db_h * service, process_h * process)
{
	s_event event;
	s_event_after_fork_data data;

	event.event_type = &EVENT_AFTER_FORK;
	event.data = &data;

	data.process = process;
	data.service = service;

	initng_event_send(&event);
	if (event.status == FAILED) {
		F_("Some plugin did fail in after fork launch.\n");
		_exit(1);
	}
}
