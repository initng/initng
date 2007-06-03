/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2007 Ismael Luceno <ismael.luceno@gmail.com>
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

#include <initng.h>
#include <initng-paths.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <alloca.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#define RUNLEVEL_PATH INITNG_ROOT "/"

INITNG_PLUGIN_MACRO;

const char *module_needs[] = {
	"runlevel",
	NULL
};

stype_h *TYPE_RUNLEVEL;
stype_h *TYPE_VIRTUAL;

static active_db_h *parse_file(char *filetoparse,
                               const char *runlevel_name, stype_h * type)
{
	active_db_h *n_service;
	char *w = NULL;
	char *a = NULL;
	char *w_depends = NULL;

	D_("parse_file(%s, %s);\n", filetoparse, runlevel_name);

	/* allocate a new service */
	if (!(n_service = initng_active_db_new(runlevel_name))) {
		free(filetoparse);
		return NULL;
	}

	n_service->type = type;

	/* copy file filename into allocated file_buf */
	if (!initng_io_open_read_close(filetoparse, &w_depends)) {
		D_("parse_file(%s): Can't open config file!\n", filetoparse);
		if (n_service->name)
			free(n_service->name);

		/* free all data entries */
		remove_all(n_service);

		if (n_service)
			free(n_service);

		free(filetoparse);
		return NULL;
	}
	w = w_depends;

	while (w[0] != '\0') {
		/* skip leading spaces */
		JUMP_SPACES(w);
		if (w[0] == '\0')
			break;

		/* skip lines, starting with '#' */
		if (w[0] == '#') {
			JUMP_TO_NEXT_ROW(w);
			continue;
		}

		a = initng_string_dup_line(&w);
		if (a) {
			D_("adding dep: \"%s\"\n", a);
			set_another_string(&NEED, n_service, a);
		}

		JUMP_TO_NEXT_ROW(w);
	}

	free(w_depends);

	set_string(&FROM_FILE, n_service, initng_toolbox_strdup(filetoparse));

	free(filetoparse);

	if (initng_active_db_register(n_service))
		return n_service;

	/* We failed to register, so free it */
	if (n_service->name)
		free(n_service->name);

	remove_all(n_service);

	if (n_service)
		free(n_service);

	return NULL;
}


/* a simple parser for a runlevel file */
static void initng_rlparser(s_event * event)
{
	char *filetoparse = NULL;
	struct stat file_stat;
	char * name;

	assert(event->event_type == &EVENT_NEW_ACTIVE);
	assert(event->data);

	name = event->data;

	D_("initng_rl_parser(%s);", name);

	filetoparse = (char *) initng_toolbox_calloc(sizeof(RUNLEVEL_PATH) +
	                                             strlen(name) + 9,
	                                             1);

	/* check /etc/initng/runlevel/name.virtual */
	strcpy(filetoparse, RUNLEVEL_PATH);
	strcat(filetoparse, name);
	strcat(filetoparse, ".virtual");

	/* FIXME: replace event->ret with ifs to check for return status */
	if (stat(filetoparse, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
		if (parse_file(filetoparse, name, TYPE_VIRTUAL))
			event->status = HANDLED;
		return;
	}

	/* check /etc/initng/runlevel/name.runlevel */
	strcpy(filetoparse, RUNLEVEL_PATH);
	strcat(filetoparse, name);
	strcat(filetoparse, ".runlevel");

	if (stat(filetoparse, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
		if (parse_file(filetoparse, name, TYPE_RUNLEVEL))
			event->status = HANDLED;
		return;
	}

	free(filetoparse);
	return;
}

int module_init(int api_version)
{
	/* initiate globals */
	TYPE_RUNLEVEL = initng_service_type_get_by_name("runlevel");
	if (!TYPE_RUNLEVEL) {
		F_("ERROR, runlevel servicetype is not found, make sure runlevel plugin is loaded.\n");
		return FALSE;
	}

	TYPE_VIRTUAL = initng_service_type_get_by_name("virtual");
	if (!TYPE_VIRTUAL) {
		F_("ERROR, virtual servicetype is not found, make sure runlevel plugin is loaded.\n");
		return FALSE;
	}

	D_("initng_rl_parser: module_init();\n");
	if (api_version != API_VERSION) {
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return FALSE;
	}

	return initng_event_hook_register(&EVENT_NEW_ACTIVE, &initng_rlparser);
}

void module_unload(void)
{
	initng_event_hook_unregister(&EVENT_NEW_ACTIVE, &initng_rlparser);
}
