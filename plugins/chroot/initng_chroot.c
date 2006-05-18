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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <initng_handler.h>
#include <initng_global.h>
#include <initng_plugin_hook.h>
#include <initng_env_variable.h>

INITNG_PLUGIN_MACRO;

s_entry CHROOT = { "chroot", STRING, NULL,
	"Chroot this path, before launching the service."
};

static int do_chroot(active_db_h * s, process_h * p __attribute__ ((unused)))
{
	const char *tmp = NULL;
	char *tmp_fixed = NULL;

	assert(s);
	assert(s->name);
	assert(p);

	D_("do_suid!\n");
	if (!(tmp = get_string(&CHROOT, s)))
	{
		D_("SUID not set!\n");
		return (TRUE);
	}

	/* fix ev.${VARIABLES} */
	tmp_fixed = fix_variables(tmp, s);

	if (chdir(tmp_fixed) == -1)
	{
		F_("Chdir %s failed with %s\n", tmp_fixed, strerror(errno));
		fix_free(tmp_fixed, tmp);
		return (FALSE);
	}
	if (chroot(tmp_fixed) == -1)
	{
		F_("Chroot %s failed with %s\n", tmp_fixed, strerror(errno));
		fix_free(tmp_fixed, tmp);
		return (FALSE);
	}
	fix_free(tmp_fixed, tmp);
	return (TRUE);
}

int module_init(int api_version)
{
	D_("module_init();\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	initng_service_data_type_register(&CHROOT);
	return (initng_plugin_hook_register(&g.A_FORK, 50, &do_chroot));
}

void module_unload(void)
{
	D_("module_unload();\n");
	initng_service_data_type_unregister(&CHROOT);
	initng_plugin_hook_unregister(&g.A_FORK, &do_chroot);
}
