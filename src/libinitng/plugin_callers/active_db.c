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
#include <time.h>

#include <initng.h>

/* called to dump active_db */
int initng_plugin_callers_active_db_dump(void)
{
	s_event event;

	event.event_type = &EVENT_DUMP_ACTIVE_DB;

	initng_event_send(&event);
	return (event.status == OK);
}

/* called to reload dump of active_db */
int initng_plugin_callers_active_db_reload(void)
{
	s_event event;

	event.event_type = &EVENT_RELOAD_ACTIVE_DB;

	initng_event_send(&event);
	return (event.status == OK);
}
