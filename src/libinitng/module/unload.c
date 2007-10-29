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

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dlfcn.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <errno.h>
#include <initng.h>

#include "local.h"

/*
 * Invoke callback, clean up module.
 */
static void unload(m_h * module)
{
	assert(module != NULL);

	/* run the unload hook */
	(*module->module_unload) ();

	/* close and free the module entry in db */
	initng_module_close_and_free(module);
}

/* XXX: more information! */
int initng_module_unload_named(const char *name)
{
	m_h *m = NULL;

	assert(name != NULL);

	D_("initng_module_unload_named(%s);\n", name);

	if (!module_is_loaded(name)) {
		F_("Not unloading module \"%s\", it is not loaded\n", name);
		return FALSE;
	}

	/* find the named module in our linked list */
	while_module_db(m) {
		if (strcmp(m->module_name, name) == 0) {
			m->marked_for_removal = TRUE;
			g.modules_to_unload = TRUE;
			return TRUE;
		}
	}

	return FALSE;
}

/* this will dlclose all arrays */
/* XXX: more information needed */

/*
 * We can simply unload modules in order, because
 */
void initng_module_unload_all(void)
{
	m_h *m, *safe = NULL;

	while_module_db_safe(m, safe)
		unload(m);

	/* reset the list, to make sure its empty */
	initng_list_init(&g.module_db.list);

	D_("initng_module_close_all()\n");
}

/*
 * We can simply unload marked modules in order.
 */
void initng_module_unload_marked(void)
{
	m_h *m, *safe = NULL;

	S_;

	while_module_db_safe(m, safe) {
		if (m->marked_for_removal == TRUE) {
			if (module_is_needed(m->module_name)) {
				F_("Not unloading module \"%s\", it is "
				   "needed\n", m->module_name);
				m->marked_for_removal = FALSE;
				continue;
			}
			D_("now unloading marked module %s.\n", m->module_name);
			unload(m);
		}
	}
}
