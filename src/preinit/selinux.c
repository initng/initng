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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#inlcude "selinux.h"


void setup_selinux(void)
{
	/* load selinux policy */
	FILE *tmp_f;

	if ((tmp_f = fopen("/selinux/enforce", "r")) == NULL &&
	    getenv("SELINUX_INIT") == NULL) {
		int enforce = -1;
		putenv("SELINUX_INIT=YES");
#ifdef OLDSELINX
		if (load_policy(&enforce) == 0) {
#else
		if (selinux_init_load_policy(&enforce) != 0 && enforce > 0) {
#endif
			/* SELinux in enforcing mode but load_policy
			 * failed. At this point, we probably can't
			 * open /dev/console, so log() won't work
			 */
			fprintf(stderr, "Enforcing mode requested but"
				" no policy loaded. Halting now.\n");
			exit(1);
		}
	} else {
		fclose(tmp_f);
	}
}
