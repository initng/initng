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

/* FIXME : avoid guessing the module name, just rely on module_open. */


#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dlfcn.h>
#include <assert.h>
#include <string.h>
#include <fnmatch.h>
#include <errno.h>

#include <sys/types.h>	/* {open,read}dir */
#include <dirent.h>	/* {open,read}dir */
#include <limits.h>	/* NAME_MAX */

#include <initng.h>
#include <initng-paths.h>

#include "local.h"


#ifndef NAME_MAX
#define NAME_MAX 255
#endif

/*
 * Read the module information from the file. Does not actually call
 * module_init() for the module, so it is not "loaded" at this point.
 */
m_h *initng_module_open(const char *module_path, const char *module_name)
{
	struct stat st;
	const char *errmsg;
	m_h *m = NULL;

	assert(module_path != NULL);
	assert(module_name != NULL);

	/* allocate, the new module info struct */
	if (!(m = (m_h *) initng_toolbox_calloc(1, sizeof(m_h)))) {
		F_("Unable to allocate memory, for new module "
		   "description.\n");
		return NULL;
	}

	/* make sure this flag is not set */
	m->flags &= ~MODULE_INITIALIZED;

	/* check that file exists */
	if (stat(module_path, &st) != 0) {
		F_("Module \"%s\" not found\n", module_path);
		free(m);
		return NULL;
	}

	/* open module */
	dlerror();		/* clear any existing error */
	m->dlhandle = dlopen(module_path, RTLD_LAZY);
	/*
	 * this breaks ngc2 on my testbox - neuron :
	 * g.modules[i].module = dlopen(module_name, RTLD_NOW | RTLD_GLOBAL);
	 * */
	if (m->dlhandle == NULL) {
		F_("Error opening module %s; %s\n", module_name, dlerror());
		free(m);
		return NULL;
	}

	D_("Success opening module \"%s\"\n", module_name);

	dlerror();		/* clear any existing error */
	m->modinfo = dlsym(m->dlhandle, "initng_module");
	if (!m->modinfo) {
		errmsg = dlerror();
		F_("Error reading initng_module struct: %s\n", errmsg);
		goto error;
	}

	if (m->modinfo->api_version != API_VERSION) {
		F_("Module %s has wrong api version, this meens that "
		   "it's compiled with another version of initng.\n",
		   module_path);
		goto error;
	}

	if (!(m->modinfo->init && m->modinfo->unload)) {
		F_("Invalid module, missing init or unload function.");
		goto error;
	}

	/* set module name in database */
	m->name = initng_toolbox_strdup(module_name);
	m->path = initng_toolbox_strdup(module_path);

	return m;

error:
	initng_module_close_and_free(m);
	return NULL;
}

/*
 * Close the module.
 */
void initng_module_close_and_free(m_h * m)
{
	assert(m != NULL);

	/* free module name */
	if (m->name) {
		/*printf("Free: %s\n", m->name); */
		free(m->name);
		m->name = NULL;
	}

	/* free module_filename */
	if (m->path) {
		free(m->path);
		m->path = NULL;
	}

	/* close the lib */
	if (m->dlhandle)
		dlclose(m->dlhandle);

	/* remove from list if added */
	initng_list_del(&m->list);

	/* free struct */
	free(m);
}

/* load a dynamic module */
/* XXX: more information! */
m_h *initng_module_load(const char *module)
{
	char *module_path = NULL;
	char *module_name = NULL;
	m_h *new_m;

	assert(module != NULL);

	/* if its a full path starting with / and contains .so */
	if (module[0] == '/' && strstr(module, ".so")) {
		int len = strlen(module);
		int i = len;

		/* then copy module name, to module_path */
		module_path = initng_toolbox_strdup(module);

		/*
		 * backwards step until we stand on
		 * first char after the last '/' char
		 */
		while (i > 1 && module[i - 1] != '/')
			i--;

		/* libsyslog.so -->> syslog.so */
		if (strncmp("lib", &module[i], 3) == 0)
			i += 3;

		/* now set module_name */
		module_name = initng_toolbox_strndup(&module[i], len - i - 3);
	} else {
		/* set modulename from module */
		module_name = (char *)module;
	}

	/* look for duplicates */
	if (module_is_loaded(module_name)) {
		F_("Module \"%s\" already loaded, won't load it twice\n",
		   module_name);

		/* free module_name if duped */
		if (module_name != module)
			free(module_name);
		if (module_path)
			free(module_path);
		return NULL;
	}

	if (!module_path) {
		/* build a path */
		module_path = (char *)initng_toolbox_calloc(1,
							    strlen
							    (INITNG_MODULE_DIR)
							    +
							    strlen(module_name)
							    + 30);
		strcpy(module_path, INITNG_MODULE_DIR "/lib");
		strcat(module_path, module_name);
		strcat(module_path, ".so");
	}

	/* load information from library */
	new_m = initng_module_open(module_path, module_name);

	/* try again with a static path */
	if (!new_m) {
		strcpy(module_path, "/lib/initng/");
		strcat(module_path, module_name);
		strcat(module_path, ".so");
		new_m = initng_module_open(module_path, module_name);

		/* try 3d time with a new static path */
		if (!new_m) {
			strcpy(module_path, "/lib32/initng/");
			strcat(module_path, module_name);
			strcat(module_path, ".so");
			new_m = initng_module_open(module_path, module_name);

			/* try 4d time with a new static path */
			if (!new_m) {
				strcpy(module_path, "/lib64/initng/");
				strcat(module_path, module_name);
				strcat(module_path, ".so");
				new_m = initng_module_open(module_path,
							   module_name);

				/* make sure this succeded */
				if (!new_m) {
					F_("Unable to load module \"%s\"\n",
					   module);
					/* free, these are duped in initng_module_open(a,b) */
					if (module_name != module)
						free(module_name);
					free(module_path);
					return NULL;
				}
			}
		}
	}
	/* free, these are duped in initng_module_open(a,b) */
	if (module_name != module)
		free(module_name);
	free(module_path);

	/* see if we have our dependencies met */
	if (!module_needs_are_loaded(new_m)) {
		F_("Not loading module \"%s\", missing needed module(s)\n",
		   module_path);
		initng_module_close_and_free(new_m);
		return NULL;
	}

	/* run module_init */
	if ((*new_m->modinfo->init)() > 0) {
		new_m->flags |= MODULE_INITIALIZED;
	} else {
		F_("Module %s did not load correctly\n", module_path);
		initng_module_close_and_free(new_m);
		return NULL;
	}

	assert(new_m->name);
	initng_list_add(&new_m->list, &g.module_db.list);
	/* and we're done */
	return new_m;
}

int initng_module_load_all(const char *path)
{
	char *modpath = NULL;
	char *module = NULL;
	int success = FALSE;

	m_h *current, *safe = NULL;

	/* memory for full path */
	modpath = initng_toolbox_calloc(strlen(path) + NAME_MAX + 2, 1);

	/* open module dir */
	DIR *pdir = opendir(path);
	if (!pdir) {
		F_("Unable to open module directory %s.\n", path);
		return FALSE;
	}

	/* get every entry */
	struct dirent *file;
	while ((file = readdir(pdir))) {
		/* skip files without the "mod" prefix */
		if (file->d_name[0] != 'm' || file->d_name[1] != 'o'
		    || file->d_name[2] != 'd')
			continue;

		/* remove suffix and prefix */
		module = initng_toolbox_strndup(file->d_name + 3,
						strlen(file->d_name + 3) - 3);

		/* check if the plugin is blacklisted */
		if (initng_common_service_blacklisted(module)) {
			F_("Module %s blacklisted.\n", module);
			free(module);
			module = NULL;
			continue;
		}

		strcpy(modpath, path);
		strcat(modpath, "/");
		strcat(modpath, file->d_name);


		current = initng_module_open(modpath, module);
		free(module);
		module = NULL;

		if (!current)
			continue;
	
		/* add to list and continue */
		assert(current->name);
		initng_list_add(&current->list, &g.module_db.list);

		/* This is true until any plugin loads sucessfully */
		success = TRUE;
	}	

	closedir(pdir);
	free(modpath);

	/* load the entries on our TODO list */
	while_module_db_safe(current, safe) {
		/* already initialized */
		if (current->flags & MODULE_INITIALIZED)
			continue;

		if (!module_needs_are_loaded(current)) {
			initng_module_close_and_free(current);
			continue;
		}

		/* if we did find find a module with needs loaded, try to load
		 * it */
		if ((*current->modinfo->init)() > 0) {
			current->flags |= MODULE_INITIALIZED;
		} else {
			F_("Module %s did not load correctly\n",
			   current->name);
			initng_module_close_and_free(current);
		}
	}

	/* initng_load_static_modules(); */
	return success;
}
