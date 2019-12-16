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

#include <initng.h>
#include <initng-paths.h>

#include <stdio.h>
#include <stdlib.h>		/* free() exit() */
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

#include "initng_reload.h"

extern char **environ;

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { "service", "daemon", "runlevel", NULL },
	.init = &module_init,
	.unload = &module_unload
};

#define SAVE_FILE		VARDIR "/initng_db_backup.v15"

#define SAVE_FILE_V13		VARDIR "/initng_db_backup.v13"

static int write_file(const char *filename);
static int read_file(const char *filename);
static int cmd_fast_reload(char *arg);


s_command FAST_RELOAD = {
	.id = 'c',
	.long_id = "hot_reload",
	.com_type = INT_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = NO_OPT,
	.u = {(void *)&cmd_fast_reload},
	.description = "Fast Reload"
};

#if 0
static int fd_used_by_service(int fd)
{
	active_db_h *service = NULL;

	while_active_db(service) {
		process_h *process = NULL;

		while_processes(process, service) {
			pipe_h *current_pipe = NULL;

			while_pipes(current_pipe, process) {
				if ((current_pipe->dir == OUT_PIPE ||
				     current_pipe->dir == BUFFERED_OUT_PIPE) &&
				    current_pipe->pipe[0] == fd) {
					W_("Wont close output_pipe fd %i, "
					   "used by service \"%s\"\n", fd,
					   service->name);
					return TRUE;
				} else if (current_pipe->dir == IN_PIPE &&
					   current_pipe->pipe[1] == fd) {
					W_("Wont close input_pipe fd %i, "
					   "used by service \"%s\"\n", fd,
					   service->name);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}
#endif

static int cmd_fast_reload(char *arg)
{
	(void)arg;
	int retval;
	char *new_argv[4];

	retval = initng_module_callers_active_db_dump();

	if (retval == TRUE) {
		D_("exec()ing initng\n");
		new_argv[0] = (char *)"/sbin/initng";
		new_argv[1] = (char *)"--hot_reload";
		new_argv[2] = NULL;

		execve(new_argv[0], new_argv, environ);
		F_("Failed to reload initng!\n");
	} else {
		if (retval == FALSE)
			F_("No module was willing to dump state\n");
		else
			F_("dump_state failed!\n");
	}

	return TRUE;
}

/* FIXME - not all errors are detected, in some cases success could be
   reported incorrectly */
static int read_file(const char *filename)
{
	FILE *fil;
	int success = TRUE;

	fil = fopen(filename, "r");

	if (!fil)
		return FALSE;

	while (!feof(fil)) {
		active_db_h *new_entry = NULL;
		s_data *d = NULL;
		data_save_struct entry;

		if (!fread(&entry, sizeof(entry), 1, fil))
			continue;

		if (initng_active_db_find_by_name(entry.name)) {
			W_("Entry exists, won't create it!\n");
			continue;
		}

		/* create a new service entry */
		if (!(new_entry = initng_active_db_new(entry.name))) {
			F_("Can't create new active!\n");
			success = FALSE;
			continue;
		}

		/* set current_state */
		new_entry->current_state =
		    initng_active_state_find(entry.state);
		if (!new_entry->current_state) {
			F_("Could not find a proper state to set: %s.\n",
			   entry.state);
			success = FALSE;
			continue;
		}

		/* set service stype */
		new_entry->type = initng_service_type_get_by_name(entry.type);
		if (!new_entry->type) {
			F_("Unknown service type %s.\n", entry.type);
			success = FALSE;
			continue;
		}

		/* set time_current_state */
		memcpy(&new_entry->time_current_state,
		       &entry.time_current_state, sizeof(struct timeval));

		/* walk through all processes */
		{
			int pnr = 0;

			while (entry.process[pnr].ptype[0] &&
			       pnr < MAX_PROCESSES) {
				process_h *process = NULL;
				ptype_h *pt = NULL;
				int p = 0;	/* used to count pipes */

				while_ptypes(pt) {
					if (strcmp(entry.process[pnr].ptype,
						   pt->name) == 0)
						break;
				}

				/* check so it was found */
				if (strcmp(entry.process[pnr].ptype, pt->name)
				    != 0) {
					F_("Unknown process type %s\n",
					   entry.process[pnr].ptype);
					pnr++;
					continue;
				}

				/* allocate the process */
				process = initng_process_db_new(pt);
				if (!process)
					continue;

				/* fill the data */
				process->pid = entry.process[pnr].pid;

				/* for every pipe */
				while (entry.process[pnr].pipes[p].dir > 0 &&
				       p < MAX_PIPES) {
					int i;
					pipe_h *op = initng_toolbox_calloc(1,
									   sizeof
									   (pipe_h));

					if (!op) {
						free(process);
						continue;
					}

					op->pipe[0] =
					    entry.process[pnr].pipes[p].pipe[0];
					op->pipe[1] =
					    entry.process[pnr].pipes[p].pipe[1];
					op->dir =
					    entry.process[pnr].pipes[p].dir;
					for (i = 0;
					     i < MAX_TARGETS
					     && i < MAX_PIPE_TARGETS
					     && entry.process[pnr].pipes[p].
					     targets[i] > 0; i++) {
						op->targets[i] =
						    entry.process[pnr].pipes[p].
						    targets[i];
					}

					add_pipe(op, process);
					p++;
				}
				process->r_code = entry.process[pnr].rcode;

				/* add this process to the list */
				initng_list_add(&process->list,
						&new_entry->processes.list);

				D_("Added process type %s to %s\n",
				   process->pt->name, new_entry->name);

				pnr++;
			}
		}

		{
			int i = 0;

			while (entry.data[i].opt_type) {
				d = (s_data *) initng_toolbox_calloc(1,
								     sizeof
								     (s_data));

				d->type =
				    initng_service_data_type_find(entry.data[i].
								  type);

				if (!d->type) {
					F_("Did not found %s!\n",
					   entry.data[i].type);
					free(d);
					i++;
					continue;
				}

				/* copy data */
				switch (d->type->type) {
				case STRING:
				case STRINGS:
				case VARIABLE_STRING:
				case VARIABLE_STRINGS:
					d->t.s =
					    initng_toolbox_strdup(entry.data[i].
								  t.s);
					break;

				case INT:
				case VARIABLE_INT:
					d->t.i = entry.data[i].t.i;
					break;

				default:
					break;
				}

				/* copy variable name if present */
				if (d->type->type > 50)
					d->vn = initng_toolbox_strdup(
							entry.data[i].vn);

				initng_list_add(&d->list,
						&new_entry->data.head.list);
				i++;
			}
		}

		/* add the new service to the active_db */
		if (initng_active_db_register(new_entry) != TRUE) {
			F_("Could not add entry!\n");
			initng_active_db_free(new_entry);
			success = FALSE;
			continue;
		}

		/* Don't need to reload data from disk, loaded when needed
		   if (get_service(new_entry) == TRUE)
		   new_entry->from_service =
		   service_db_find_by_name(new_entry->name);
		 */
	}

	fclose(fil);
	if (unlink(filename) != 0) {
		W_("Failed removing file %s !!!\n", filename);
		return success;	/* not important */
	}

	return success;
}

static int read_file_v13(const char *filename)
{
	FILE *fil;
	int success = TRUE;

	fil = fopen(filename, "r");

	if (!fil)
		return FALSE;

	while (!feof(fil)) {
		active_db_h *new_entry = NULL;
		s_data *d = NULL;
		data_save_struct_v13 entry;

		if (!fread(&entry, sizeof(entry), 1, fil))
			continue;

		if (initng_active_db_find_by_name(entry.name)) {
			W_("Entry exists, won't create it!\n");
			continue;
		}

		/* create a new service entry */
		if (!(new_entry = initng_active_db_new(entry.name))) {
			F_("Can't create new active!\n");
			success = FALSE;
			continue;
		}

		/* set current_state */
		new_entry->current_state =
		    initng_active_state_find(entry.state);
		if (!new_entry->current_state) {
			F_("Could not find a proper state to set: %s.\n",
			   entry.state);
			success = FALSE;
			continue;
		}

		/* set service stype */
		if (!(new_entry->type =
		      initng_service_type_get_by_name(entry.type))) {
			F_("Unknown service type %s.\n", entry.type);
			success = FALSE;
			continue;
		}

		/* set time_current_state */
		memcpy(&new_entry->time_current_state,
		       &entry.time_current_state, sizeof(struct timeval));

		/* walk through all processes */
		{
			int pnr = 0;

			while (entry.process[pnr].ptype[0] &&
			       pnr < MAX_PROCESSES) {
				process_h *process = NULL;
				ptype_h *pt = NULL;
				int p = 0;	/* used to count pipes */

				while_ptypes(pt) {
					if (strcmp(entry.process[pnr].ptype,
						   pt->name) == 0)
						break;
				}

				/* check so it was found */
				if (strcmp(entry.process[pnr].ptype,
					   pt->name) != 0) {
					F_("Unknown process type %s\n",
					   entry.process[pnr].ptype);
					pnr++;
					continue;
				}

				/* allocate the process */
				process = initng_process_db_new(pt);
				if (!process)
					continue;

				/* fill the data */
				process->pid = entry.process[pnr].pid;

				/* for every pipe */
				{
					pipe_h *op = initng_toolbox_calloc(1,
									   sizeof
									   (pipe_h));

					if (!op) {
						free(process);
						continue;
					}

					op->pipe[0] =
					    entry.process[pnr].stdout1;
					op->pipe[1] =
					    entry.process[pnr].stdout2;
					op->dir = BUFFERED_OUT_PIPE;
					op->targets[0] = 1;
					op->targets[1] = 2;
					add_pipe(op, process);
					p++;
				}
				process->r_code = entry.process[pnr].rcode;

				/* add this process to the list */
				initng_list_add(&process->list,
						&new_entry->processes.list);

				D_("Added process type %s to %s\n",
				   process->pt->name, new_entry->name);

				pnr++;
			}
		}

		{
			int i = 0;

			while (entry.data[i].opt_type) {
				d = (s_data *) initng_toolbox_calloc(1,
								     sizeof
								     (s_data));
				d->type =
				    initng_service_data_type_find(entry.data[i].
								  type);
				if (!d->type) {
					F_("Did not found %s!\n",
					   entry.data[i].type);
					free(d);
					i++;
					continue;
				}

				/* copy data */
				switch (d->type->type) {
				case STRING:
				case STRINGS:
				case VARIABLE_STRING:
				case VARIABLE_STRINGS:
					d->t.s =
					    initng_toolbox_strdup(entry.data[i].
								  t.s);
					break;

				case INT:
				case VARIABLE_INT:
					d->t.i = entry.data[i].t.i;
					break;

				default:
					break;
				}

				d->vn = NULL;

				initng_list_add(&d->list,
						&new_entry->data.head.list);
				i++;
			}
		}

		/* add the new service to the active_db */
		if (initng_active_db_register(new_entry) != TRUE) {
			F_("Could not add entry!\n");
			initng_active_db_free(new_entry);
			success = FALSE;
			continue;
		}

		/* Don't need to reload data from disk, loaded when needed
		   if (get_service(new_entry) == TRUE)
		   new_entry->from_service =
		   service_db_find_by_name(new_entry->name);
		 */
	}

	fclose(fil);
	if (unlink(filename) != 0) {
		W_("Failed removing file %s !!!\n", filename);
		return success;	/* not important */
	}

	return success;
}

static int write_file(const char *filename)
{
	FILE *fil;
	active_db_h *current, *q = NULL;
	data_save_struct entry;
	process_h *process = NULL;
	pipe_h *current_pipe = NULL;
	int i;
	int pnr = 0;
	s_data *c_d = NULL;
	int success = TRUE;

	fil = fopen(filename, "w+");
	if (!fil) {
		F_("Could not open '%s' for writing\n", filename);
		return FALSE;
	}

	/* walk the active_db */
	while_active_db_safe(current, q) {
		if (!current->current_state) {
			F_("State is not set, wont save this one!\n");
			continue;
		}

		if (strlen(current->name) >= MAX_SERVICE_NAME_STRING_LEN) {
			F_("Service name is to long, it won't fit the "
			   "fileformat spec! max is %i, won't save this "
			   "service!\n", MAX_SERVICE_NAME_STRING_LEN);
			success = FALSE;
			continue;
		}

		memset(&entry, 0, sizeof entry);
		strncpy(entry.name, current->name, MAX_SERVICE_NAME_STRING_LEN);
		strncpy(entry.state, current->current_state->name,
			MAX_SERVICE_STATE_LEN);
		if (current->type)
			strncpy(entry.type, current->type->name,
				MAX_TYPE_STRING_LEN);

		memcpy(&entry.time_current_state,
		       &current->time_current_state, sizeof(struct timeval));

		/* collect some processes */
		process = NULL;
		pnr = 0;
		while_processes(process, current) {
			int p = 0;

			strncpy(entry.process[pnr].ptype, process->pt->name,
				MAX_PTYPE_STRING_LEN);
			entry.process[pnr].pid = process->pid;

			current_pipe = NULL;
			while_pipes(current_pipe, process) {
				entry.process[pnr].pipes[p].pipe[0] =
				    current_pipe->pipe[0];
				entry.process[pnr].pipes[p].pipe[1] =
				    current_pipe->pipe[1];
				entry.process[pnr].pipes[p].dir =
				    current_pipe->dir;
				for (i = 0; i < MAX_TARGETS &&
				     i < MAX_PIPE_TARGETS &&
				     current_pipe->targets[i] > 0; i++) {
					entry.process[pnr].pipes[p].targets[i]
					    = current_pipe->targets[i];
				}
				p++;
			}

			entry.process[pnr].rcode = process->r_code;
			pnr++;
			if (pnr >= MAX_PROCESSES)
				break;
		}
		entry.process[pnr].ptype[0] = '\0';

		/* reset data */
		for (i = 0; i < MAX_ENTRYS_FOR_SERVICE; i++) {
			entry.data[i].opt_type = 0;
			entry.data[i].type[0] = '\0';
			entry.data[i].t.i = 0;
		}

		i = 0;
		initng_list_foreach(c_d, &current->data.head.list, list) {
			if (!c_d->type)
				continue;

			if (!c_d->type->name)
				continue;

			entry.data[i].opt_type = c_d->type->type;
			strncpy(entry.data[i].type, c_d->type->name,
				MAX_TYPE_STRING_LEN);

			/* copy the data */
			switch (c_d->type->type) {
			case STRING:
			case STRINGS:
			case VARIABLE_STRING:
			case VARIABLE_STRINGS:
				strncpy(entry.data[i].t.s, c_d->t.s,
					MAX_DATA_STRING_LEN);
				break;

			case INT:
			case VARIABLE_INT:
				entry.data[i].t.i = c_d->t.i;
				break;

			default:
				break;
			}

			/* if they have variable names */
			if (c_d->type->type > 50)
				strncpy(entry.data[i].vn, c_d->vn,
					MAX_DATA_VN_LEN);

			/* save in next data entry row */
			i++;

			/* maximum data entries that can be saved */
			if (i == MAX_ENTRYS_FOR_SERVICE) {
				F_("Maximum %d data entries '%s' service can't "
				   "be saved!\n", MAX_ENTRYS_FOR_SERVICE, entry.name);
				success = FALSE;
				break;
			}
		}

		D_("Saving : %s\n", entry.name);
		if (fwrite(&entry, sizeof(entry), 1, fil) != 1) {
			F_("failed to write entry '%s': %m\n", entry.name);
			/* TODO: database recovery?? */
			success = FALSE;
			break;
		}
	}

	fclose(fil);
	return success;
}

static void dump_state(s_event * event)
{
	const char *file = NULL;

	assert(event->event_type == &EVENT_DUMP_ACTIVE_DB);

	/* set the correct filename */
	file = SAVE_FILE;

	/* if there is a file */
	if (!file)
		return;

	if (write_file(file) != TRUE)
		event->status = FAILED;
}

static void reload_state(s_event * event)
{
	struct stat st;
	const char *file = NULL;

	assert(event->event_type == &EVENT_RELOAD_ACTIVE_DB);

	/* set the correct filename */
	file = SAVE_FILE;

	/* if there is a file */
	if (!file)
		return;

	/* check that file exits */
	if (stat(file, &st) == 0) {
		if (!read_file(file))
			event->status = FAILED;
		return;
	}

	/* set the correct filename for import of v13 statefiles */
	file = SAVE_FILE_V13;

	/* check that file exits */
	if (stat(file, &st) == 0) {
		if (!read_file_v13(file))
			event->status = FAILED;
		return;
	}

	D_("No state file found, passing on reload_state request\n");
}

/* Save a reload file for backup if initng segfaults */
static void save_backup(s_event * event)
{
	h_sys_state *state;

	assert(event->event_type == &EVENT_SYSTEM_CHANGE);
	assert(event->data);

	state = event->data;

	/* only save when system is getting up */
	if (*state == STATE_UP) {
		/* save file */
		write_file(SAVE_FILE);
		return;
	}

	/* if system is stopping, remove the SAVE_FILE */
	if (*state == STATE_STOPPING) {
		unlink(SAVE_FILE);
	}
}

int module_init(void)
{
	/*    if (g.hot_reload) {
	   read_file(SAVE_FILE);
	   } */

	initng_event_hook_register(&EVENT_SYSTEM_CHANGE, &save_backup);
	initng_event_hook_register(&EVENT_DUMP_ACTIVE_DB, &dump_state);
	initng_event_hook_register(&EVENT_RELOAD_ACTIVE_DB, &reload_state);
	initng_command_register(&FAST_RELOAD);
	return TRUE;
}

void module_unload(void)
{
	initng_event_hook_unregister(&EVENT_SYSTEM_CHANGE, &save_backup);
	initng_event_hook_unregister(&EVENT_DUMP_ACTIVE_DB, &dump_state);
	initng_event_hook_unregister(&EVENT_RELOAD_ACTIVE_DB, &reload_state);
	initng_command_unregister(&FAST_RELOAD);
}
