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
#include "initng_load_module.h"
#include "initng_toolbox.h"
#include "initng_signal.h"
#include "initng_static_event_types.h"
#include "initng_common.h"
#include "initng_plugin_callers.h"
#include "initng_plugin.h"
#include "initng_event.h"


active_db_h *initng_plugin_create_new_active(const char *name)
{
	s_event event;

	event.event_type = &EVENT_NEW_ACTIVE;
	event.data = (void *) name;

	initng_event_send(&event);
	if (event.status == HANDLED)
		return event.ret;

	return (NULL);
}
