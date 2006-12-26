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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>							/* fcntl() */
#include <time.h>
#include "initng.h"
#include "initng_global.h"
//#include "initng_service_cache.h"
#include "initng_load_module.h"
#include "initng_toolbox.h"
#include "initng_signal.h"
#include "initng_static_event_types.h"
#include "initng_common.h"

#include "initng_plugin_callers.h"
#include "initng_plugin.h"


active_db_h *initng_plugin_create_new_active(const char *name)
{
	s_call *current, *q = NULL;
	active_db_h *ret = NULL;

	while_list_safe(current, &g.NEW_ACTIVE, q)
	{
		ret = (*current->c.new_active) (name);
		if (ret)
			return (ret);
	}

	return (NULL);
}


void initng_plugin_callers_signal(int signal)
{
	s_call *current, *q = NULL;

	while_list_safe(current, &g.SIGNAL, q)
	{
		(*current->c.signal_hook) (signal);
	}

}

int initng_plugin_callers_handle_killed(active_db_h * s, process_h * p)
{
	s_call *current, *q = NULL;

	while_list_safe(current, &g.HANDLE_KILLED, q)
	{
		D_("Calling killed_handle plugin from %s\n", current->from_file);
		if (current->c.handle_killed)
			if (((*current->c.handle_killed) (s, p)) == TRUE)
				return (TRUE);
	}
	return (FALSE);
}


void initng_plugin_callers_compensate_time(int t)
{
	s_call *current, *q = NULL;

	while_list_safe(current, &g.COMPENSATE_TIME, q)
	{
		if (current->c.swatcher)
		{
			D_("Calling system_state_changed plugin from %s\n",
			   current->from_file);
			(*current->c.swatcher) (t);
		}
	}

}

/* called when system state has changed. */
void initng_plugin_callers_load_module_system_changed(h_sys_state state)
{
	s_event event;

	event.event_type = &EVENT_SYSTEM_CHANGE;
	event.data = &state;
	event.break_on = FAIL;

	initng_event_send(&event);

	return;
}

/* called to dump active_db */
int initng_plugin_callers_dump_active_db(void)
{
	s_call *current, *q = NULL;
	int retval = FALSE;

	while_list_safe(current, &g.DUMP_ACTIVE_DB, q)
	{
		if (current->c.dump_active_db)
		{
			D_("Calling dump_active_db plugin from %s\n", current->from_file);
			retval = (*current->c.dump_active_db) ();
			if (retval == TRUE)
			{
				D_("dump_active_db plugin from %s succeeded\n",
				   current->from_file);
				break;
			}
			else if (retval == FAIL)
			{
				F_("dump_state plugin from %s failed\n", current->from_file);
				break;
			}
		}
	}
	return retval;

}

/* called to reload dump of active_db */
int initng_plugin_callers_reload_active_db(void)
{
	s_call *current, *q = NULL;
	int retval = FALSE;

	while_list_safe(current, &g.RELOAD_ACTIVE_DB, q)
	{
		if (current->c.reload_active_db)
		{
			D_("Calling reload_active_db plugin from %s\n",
			   current->from_file);
			retval = (*current->c.reload_active_db) ();
			if (retval == TRUE)
			{
				D_("reload_active_db plugin from %s succeeded\n",
				   current->from_file);
				break;
			}
			else if (retval == FAIL)
			{
				F_("reload_active_db plugin from %s failed\n",
				   current->from_file);
				break;
			}
		}
	}

	return retval;
}
