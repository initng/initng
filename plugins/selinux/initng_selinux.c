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

#include <assert.h>

#include <initng_global.h>
#include <initng_active_state.h>
#include <initng_active_db.h>
#include <initng_process_db.h>
#include <initng_service_cache.h>
#include <initng_handler.h>
#include <initng_active_db.h>
#include <initng_toolbox.h>
#include <initng_plugin_hook.h>
#include <initng_load_module.h>
#include <initng_plugin_callers.h>
#include <initng_error.h>
#include <initng_plugin.h>
#include <initng_static_states.h>
#include <initng_control_command.h>

#include <initng-paths.h>

#include <stdlib.h>
#include <string.h>
#include <selinux/selinux.h>
#include <selinux/context.h>
#include <sepol/sepol.h>

INITNG_PLUGIN_MACRO;

s_entry SELINUX_CONTEXT = { "selinux_context", STRING, NULL,
	"The selinux context to start in."
};

static int set_selinux_context(active_db_h * s, process_h * p
							   __attribute__ ((unused)))
{
	const char *selinux_context = get_string(&SELINUX_CONTEXT, s);
	char *sestr = NULL;
	context_t seref = NULL;
	int rc = 0;
	char *sedomain;
	int enforce = -1;
	/*static s_entry * SCRIPT = NULL;
	SCRIPT = initng_service_data_type_find("script");
	if(!is_var(SCRIPT, p->pt->name, s)) {
		return (TRUE);
	} */
	if (selinux_context)
	{
		sedomain = (char *) malloc((sizeof(char) * strlen(selinux_context) +
									1));
		strcpy(sedomain, selinux_context);
	}
	else
	{
		sedomain = (char *) malloc((sizeof(char) * 9));
		strcpy(sedomain, "initrc_t");
	}
	rc = getcon(&sestr);
	if (rc < 0)
		goto fail;
	seref = context_new(sestr);
	if (!seref)
		goto fail;
	if (context_type_set(seref, sedomain))
		goto fail;
	freecon(sestr);
	sestr = context_str(seref);
	if (!sestr)
		goto fail;
	rc = setexeccon(sestr);
	if (rc < 0)
		goto fail;
	return (TRUE);

  fail:
	selinux_getenforcemode(&enforce);
	if (enforce)
	{
		F_("bash_this(): could not change selinux context!\n ERROR!\n");
		return (FALSE);
	}
	else
	{
		return (TRUE);
	}
}


int module_init(int api_version)
{
	D_("module_init();\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}
	/*
	if(g.i_am != I_AM_INIT) { 
		F_("Selinx have no effect in fake mode\n"); 
		return(FALSE); 
	}
	*/
	/* add hooks and malloc data here */

	initng_service_data_type_register(&SELINUX_CONTEXT);
	initng_plugin_hook_register(&g.A_FORK, 20, &set_selinux_context);

	return (TRUE);
}


void module_unload(void)
{
	D_("module_unload();\n");

	/* remove hooks and free data here */
	initng_service_data_type_unregister(&SELINUX_CONTEXT);
	initng_plugin_hook_unregister(&g.A_FORK, &set_selinux_context);
}
