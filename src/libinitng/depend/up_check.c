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

#include "initng.h"

#include <stdio.h>
#include <stdlib.h>							/* free() exit() */
#include <string.h>
#include <assert.h>

#include "initng_handler.h"
#include "initng_global.h"
#include "initng_common.h"
#include "initng_toolbox.h"
#include "initng_static_data_id.h"
#include "initng_static_states.h"
#include "initng_env_variable.h"
#include "initng_static_event_types.h"

#include "initng_depend.h"


/*
 * This is a final check, before a daemon or service can be marked as UP.
 */
int initng_depend_up_check(active_db_h * service)
{
	s_event event;

	event.event_type = &EVENT_UP_MET;
	event.data = service;

	return (initng_event_send(&event));
}
