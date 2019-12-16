/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Elan Ruusamäe <glen@pld-linux.org>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};


s_entry LOCKFILE = {
	.name = "lockfile",
	.description = "If this option is set, module will create "
		       "/var/lock/subsys/$NAME lockfile.",
	.type = SET,
	.ot = NULL,
};

#define LOCKDIR "/var/lock/subsys/"

static void status_change(s_event * event)
{
	active_db_h *service;

	assert(event->event_type == &EVENT_IS_CHANGE);
	assert(event->data);

	service = event->data;

	D_("status change [%s]\n", service->name);

	/* are we under influence of lockfile? */
	if (is(&LOCKFILE, service)) {
		char *p = strrchr(service->name, '/') + 1;
		char lockfile[sizeof(LOCKDIR) + strlen(p)];

		memcpy(lockfile, LOCKDIR, sizeof(LOCKDIR) - 1);
		strcpy(lockfile + sizeof(LOCKDIR) - 1, p);

		D_("lockfile path [%s]\n", lockfile);
		/* service states from initng_is.h */
		switch (GET_STATE(service)) {
		case IS_UP:
			D_("service got up\n");
			int fd = creat(lockfile, 0640);
			if (fd != -1)
				close(fd);
			break;

		case IS_DOWN:
			D_("service went down\n");
			unlink(lockfile);
			break;

		default:
			break;
		}
	}
}

int module_init(void)
{
	initng_service_data_type_register(&LOCKFILE);
	initng_event_hook_register(&EVENT_IS_CHANGE, &status_change);
	return TRUE;
}

void module_unload(void)
{
	initng_service_data_type_unregister(&LOCKFILE);
	initng_event_hook_unregister(&EVENT_IS_CHANGE, &status_change);
}
