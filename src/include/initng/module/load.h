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

#ifndef INITNG_LOAD_MODULE_H
#define INITNG_LOAD_MODULE_H

#include <initng/config/all.h>
#include <initng/active_db.h>
#include <initng/system_states.h>
#include <initng/module/module.h>

/* public interface */
m_h *initng_module_load(const char *module_path);
int initng_module_unload_named(const char *name);
int initng_module_load_all(const char *plugin_path);
void initng_module_unload_all(void);
void initng_module_unload_marked(void);

/* functions for internal use only (exposed for testing) */
m_h *initng_module_open(const char *module_path,
			const char *module_name);
void initng_module_close_and_free(m_h * m);

#endif /* INITNG_LOAD_MODULE_H */
