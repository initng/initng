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

#ifndef INITNG_H
#define INITNG_H

#include <misc.h>
#include <active_db.h>
#include <active_state.h>
#include <common.h>
#include <control_command.h>
#include <depend.h>
#include <env_variable.h>
#include <error.h>
#include <event.h>
#include <execute.h>
#include <fd.h>
#include <fork.h>
#include <global.h>
#include <handler.h>
#include <interrupt.h>
#include <is_state.h>
#include <kill_handler.h>
#include <list.h>
#include <load_module.h>
#include <main.h>
#include <module.h>
#include <msg.h>
#include <open_read_close.h>
#include <plugin_callers.h>
#include <plugin.h>
#include <process_db.h>
#include <service_data_types.h>
#include <service_types.h>
#include <signal.h>
#include <static_data_id.h>
#include <static_event_types.h>
#include <static_service_types.h>
#include <static_states.h>
#include <string_tools.h>
#include <struct_data.h>
#include <system_states.h>
#include <toolbox.h>

#endif /* INITNG_H */
