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
#include <sys/stat.h>
#include <stdio.h>
#include <dlfcn.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <errno.h>
#include "initng.h"
#include "initng_global.h"
#include "initng_load_module.h"
#include "initng_module.h"
#include "initng_toolbox.h"
#include "initng_main.h"
#include "initng_common.h"
#include <initng-paths.h>


/*
 * Some modules depend on other modules.
 *
 * A module declares its dependencies by adding an array called
 * module_needs, with the names of the modules that it needs, like
 * this:
 *
 * char *module_needs[] = {
 *     "foo",
 *     "bar",
 *     NULL
 * };
 *
 * When a module is loaded, it checks to see if all of the modules
 * that it depends on are already loaded. If they are not, it refuses
 * to load.
 *
 * The initng_load_all_modules() routine will insure that modules are
 * loaded in the correct order. It does this by loading all modules
 * that have their dependencies met, and iterating until either all
 * modules are loaded or there are no modules left to load. (This is
 * effectively a breadth-first search.)
 *
 * Unloading follows the same logic, except in reverse. A module may
 * not be unloaded if a module that depends on it is loaded. Likewise,
 * the initng_unload_all_modules() routine will unload modules in the
 * proper order. (Again, through a breadth-first search.)
 *
 * Modules that result in circular dependencies can never be loaded.
 *
 * Several things are missing from the dependency setup:
 *
 * - Some notion of "services". A module can depend on only specific
 *   module, say "cpout", not "any module that provides output".
 *
 * - Priority mechanism. The callback functions from modules are
 *   invoked in the order that they are loaded.
 *
 * - Ability for modules to refuse to unload.
 *
 * Hopefully these will not matter!
 */

/* Note: modules are added to the *start* of the linked list when they
 * are loaded. This makes unloading all modules easy, because a module
 * always sits before the modules that it depends on in the list. */

/*
 * See if we have already loaded a module with the given name.
 *
 * Returns TRUE if already loaded, else FALSE.
 */
static int initng_load_module_is_loaded(const char *module_name)
{
	m_h *m = NULL;

	assert(module_name != NULL);

	while_module_db(m)
	{
		if (strcmp(m->module_name, module_name) == 0 ||
			strcmp(m->module_filename, module_name) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * Returns TRUE if module has its dependencies met, else FALSE.
 */
static int initng_load_module_needs_are_loaded(const m_h * m)
{
	char **needs;
	int retval;

	assert(m != NULL);

	/* if there are no needs, then we have met them */
	if (m->module_needs == NULL)
	{
		return TRUE;
	}

	/* otherwise check each one */
	needs = m->module_needs;
	retval = TRUE;
	if (needs != NULL)
	{
		while (*needs != NULL)
		{
			if (!initng_load_module_is_loaded(*needs))
			{
				F_("Plugin \"%s\" (%s) requires plugin \"%s\" to work, unlodading %s.\n", m->module_name, m->module_filename, *needs, m->module_name);
				retval = FALSE;
			}
			needs++;
		}
	}
	return retval;
}

/*
 * See the named module is needed elsewhere.
 *
 * Returns TRUE if the module is needed, else FALSE.
 */
static int initng_load_module_is_needed(const char *module_name)
{
	char **needs;
	m_h *m = NULL;
	int retval = FALSE;

	assert(module_name != NULL);

	while_module_db(m)
	{
		/* if not this module, have needs set, continue.. */
		if (!(m->module_needs))
			continue;
		needs = m->module_needs;

		while (*needs != NULL)
		{
			if (strcmp(module_name, *needs) == 0)
			{
				F_("Module \"%s\" needed by \"%s\"\n", module_name,
				   m->module_name);
				retval = TRUE;
			}
			needs++;
		}
	}
	return retval;
}
