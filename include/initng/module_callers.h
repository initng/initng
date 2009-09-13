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

#ifndef INITNG_MODULE_CALLERS_H
#define INITNG_MODULE_CALLERS_H

#include <initng/active_db.h>
#include <initng/system_states.h>
#include <initng/config/all.h>

#include <stdarg.h>

active_db_h *initng_plugin_active_new(const char *name);
int initng_plugin_callers_handle_killed(active_db_h * s, process_h * p);
void initng_plugin_callers_compensate_time(time_t t);
void initng_plugin_callers_signal(int signal);

void initng_plugin_callers_system_changed(h_sys_state state);
int initng_plugin_callers_active_db_dump(void);
int initng_plugin_callers_active_db_reload(void);

#endif /* INITNG_MODULE_CALLERS_H */
