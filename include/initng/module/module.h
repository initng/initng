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

#ifndef INITNG_MODULE_H
#define INITNG_MODULE_H

#include <initng/list.h>

/* Add to this counter everytime the api changes, and plugins need to
 * recompile */
#define API_VERSION 19

/* define this macro in start of every plugin to check api version */
#define INITNG_MODULE(init, unload, ...) extern struct initng_module initng_module = { API_VERSION, { ... }, init, unload};

struct initng_module {
	int  api_version;
	char **deps;
	int  (*init) (int api_version);
	void (*unload) (void);
};


typedef struct module_struct {
	char *name;
	char *path;
	void *dlhandle;

	enum {
		MODULE_INITIALIZED = 1,
		MODULE_REMOVE      = 2
	} flags;

	struct initng_module *modinfo;

	list_t list;
} m_h;

#define while_module_db(current) \
	initng_list_foreach_rev(current, &g.module_db.list, list)

#define while_module_db_safe(current, safe) \
	initng_list_foreach_rev_safe(current, safe, &g.module_db.list, list)

#endif /* INITNG_MODULE_H */
