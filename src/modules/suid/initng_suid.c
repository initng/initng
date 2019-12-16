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

#define _DEFAULT_SOURCE	/* initgroups() */

#include <initng.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pwd.h>

#include <grp.h>
#include <sys/types.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { NULL },
	.init = &module_init,
	.unload = &module_unload
};

s_entry SUID = {
	.name = "suid",
	.description = "Set this user id on launch.",
	.type = STRING,
	.ot = NULL,
};

s_entry SGID = {
	.name = "sgid",
	.description = "Set this group id on launch.",
	.type = STRING,
	.ot = NULL,
};

void adjust_env(active_db_h * service, const char *vn, const char *vv);
void adjust_env(active_db_h * service, const char *vn, const char *vv)
{
	/* add to service cache */
	if (is_var(&ENV, vn, service)) {
		return;		/* Assume they were set by .i file,
				 * don't override */
	} else {
		set_string_var(&ENV, initng_toolbox_strdup(vn), service,
			       initng_toolbox_strdup(vv));
	}
}

static void do_suid(s_event * event)
{
	s_event_after_fork_data *data;

	struct passwd *passwd = NULL;
	struct group *group = NULL;

	/* NOTE: why we have ret here? */
	int ret = TRUE;

	int gid = 0;
	int uid = 0;
	const char *groupname = NULL;
	const char *username = NULL;

	assert(event->event_type == &EVENT_AFTER_FORK);
	assert(event->data);

	data = event->data;

	assert(data->service);
	assert(data->service->name);

	groupname = get_string(&SGID, data->service);
	username = get_string(&SUID, data->service);

	if (groupname)
		group = getgrnam(groupname);

	if (username)
		passwd = getpwnam(username);

	if (passwd) {
		uid = passwd->pw_uid;
		gid = passwd->pw_gid;
	} else if (username) {
		F_("USER \"%s\" not found!\n", username);
		ret += 2;
	}

	if (group) {
		gid = group->gr_gid;
	} else if (groupname) {
		F_("GROUP \"%s\" not found!\n", groupname);
		ret++;
	}

	if (gid) {
		D_("Change to gid %i", gid);
		if (setgid(gid)) {
			F_("Cannot change to gid %i", gid);
		}
	}

	if (passwd)
		initgroups(passwd->pw_name, passwd->pw_gid);

	if (uid) {
		D_("Change to uid %i", uid);
		if (setuid(uid)) {
			F_("Cannot change to uid %i", uid);
		}

		/* Set UID-related env variables */
		adjust_env(data->service, "USER", passwd->pw_name);
		adjust_env(data->service, "HOME", passwd->pw_dir);
		adjust_env(data->service, "PATH", "/bin:/usr/bin");
	}

	/* group and passwd are static data structures - don't free */
}

int module_init(void)
{
	initng_service_data_type_register(&SUID);
	initng_service_data_type_register(&SGID);
	return (initng_event_hook_register(&EVENT_AFTER_FORK, &do_suid));
}

void module_unload(void)
{
	initng_service_data_type_unregister(&SUID);
	initng_service_data_type_unregister(&SGID);
	initng_event_hook_unregister(&EVENT_AFTER_FORK, &do_suid);
}
