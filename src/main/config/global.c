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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <initng.h>

/* HERE IS THE GLOBAL DEFINED STRUCT, WE OFTEN RELATE TO */
s_global g;

/**
 * This function initziates the global data struct, with some standard values.
 * This must be set before libinitng can be used in any way
 */
void initng_config_global_new(int argc, char *argv[], char *env[])
{
	assert(argv);
	assert(env);

	/* Zero the complete s_global */
	memset(&g, 0, sizeof(s_global));

	/* We want to keep a copy of the arguments passed to us, this will be
	 * overwritten by set_title()
	 */
	g.Argv0 = argv[0];
	g.Argv = (char **)initng_toolbox_calloc(argc + 1, sizeof(char *));
	assert(g.Argv);

	for (int i = 0; i < argc; i++) {
		g.Argv[i] = initng_toolbox_strdup(argv[i]);
	}
	g.Argv[argc] = NULL;

	/* We want to change our process name, because it looks nice in
	 * "ps" output. But we can only use space that the kernel
	 * allocated for us when we started. This is argv[] and env[]
	 * together.
	 */
	g.maxproclen = 0;

	for (char **p = argv; *p; p++) {
		g.maxproclen += strlen(*p) + 1;
	}

	for (char **p = env; *p; p++) {
		g.maxproclen += strlen(*p) + 1;
	}

	D_("Maximum length for our process name is %d\n", g.maxproclen);

	/* Initialize global data storage.
	 */
	DATA_HEAD_INIT(&g.data);

	/* Initialize all databases.
	 * next and prev have to point to own struct.
	 */
	initng_list_init(&g.active_db.list);
	initng_list_init(&g.active_db.interrupt);
	initng_list_init(&g.states.list);
	initng_list_init(&g.ptypes.list);
	initng_list_init(&g.module_db.list);
	initng_list_init(&g.option_db.list);
	initng_list_init(&g.event_db.list);
	initng_list_init(&g.command_db.list);
	initng_list_init(&g.stypes.list);

	/* Default global variables (cleared by memset above):
	 * g.interrupt
	 * g.sys_state
	 * g.runlevel
	 * g.dev_console
	 * g.modules_to_unload
	 * g.hot_reload
	 * g.verbose
	 * g.verbose_this[]
	 */

	/* Add defaults (static) to data_types */
	initng_static_data_id_register_defaults();
	/* Add static stats to g.states */
	initng_static_states_register_defaults();
	/* Add static stypes */
	initng_static_stypes_register_defaults();
	/* Add static event types */
	initng_register_static_event_types();
}

void initng_config_global_free(void)
{
	/* free all databases */
	initng_active_db_free_all();	/* clean process_db */
	initng_service_data_type_unregister_all();	/* clean option_db */
	initng_event_type_unregister_all();	/* clean event_db */
	initng_command_unregister_all();	/* clean command_db */

	/* free dynamic global variables */
	remove_all(&g);

	/* free runlevel name string */
	free(g.runlevel);

	/* free console string if set */
	free(g.dev_console);

	/* free our copy of argv, and argc */
	for (char **p = g.Argv; *p; p++) {
		free(*p);
	}
	free(g.Argv);
}
