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
#include <sys/un.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* free() exit() */
#include <assert.h>

#include <initng.h>

/*
 * this function walks through the g.Argv, if it founds service name,
 * with an starting -service, this function will return FALSE, and
 * the service wont be able to load, means that is it blacklisted.
 * return TRUE if service is okay to load.
 */
int initng_common_service_blacklisted(const char *name)
{
	int i;

	assert(name);
	assert(g.Argv);

	/* walk through arguments looking for this dep to be blacklisted */
	for (i = 1; (g.Argv)[i]; i++) {
		/* if we got a match */
		if ((g.Argv)[i][0] == '-') {
			if (strcmp(name, (g.Argv)[i] + 1) == 0 ||
			    initng_string_match(name, (g.Argv)[i] + 1))
				return TRUE;
		}
	}
	return FALSE;
}
