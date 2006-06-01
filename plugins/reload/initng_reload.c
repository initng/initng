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

#include <initng.h>


#include <stdio.h>
#include <stdlib.h>							/* free() exit() */
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

#include <initng_handler.h>
#include <initng_global.h>
#include <initng_plugin_hook.h>
#include <initng_common.h>
#include <initng_toolbox.h>
#include <initng_static_data_id.h>
#include <initng_active_state.h>
#include <initng_process_db.h>
#include <initng_service_types.h>
#include <initng-paths.h>
#include <initng_plugin_callers.h>

#include "initng_reload.h"

INITNG_PLUGIN_MACRO;

#define SAVE_FILE      VARDIR "/initng_db_backup.v13"
#define SAVE_FILE_FAKE VARDIR "/initng_db_backup_fake.v13"

static int write_file(const char *filename);
static int read_file(const char *filename);
static void cmd_fast_reload(char *arg);

const char *module_needs[] = {
	"rlparser",
	"iparser",
	"service",
	"daemon",
	"runlevel",
	NULL
};

s_command FAST_RELOAD = { 'c', "hot_reload", VOID_COMMAND, STANDARD_COMMAND, NO_OPT,
	{(void *) &cmd_fast_reload},
	"Fast Reload"
};

static int fd_used_by_service(int fd)
{
	active_db_h *service = NULL;

	while_active_db(service)
	{
		process_h *process = NULL;

		while_processes(process, service)
		{
			pipe_h *current_pipe = NULL;

			while_pipes(current_pipe, process)
			{
				if ((current_pipe->dir == OUT_PIPE
					 || current_pipe->dir == BUFFERED_OUT_PIPE)
					&& current_pipe->pipe[0] == fd)
				{
					W_("Wont close output_pipe fd %i, used by service \"%s\"\n", fd, service->name);
					return (TRUE);
				}
				else if (current_pipe->dir == IN_PIPE
						 && current_pipe->pipe[1] == fd)
				{
					W_("Wont close input_pipe fd %i, used by service \"%s\"\n", fd, service->name);
					return (TRUE);
				}
			}
		}
	}
	return (FALSE);
}

static void cmd_fast_reload(char *arg)
{
	(void) arg;
	int retval;
	int i;
	char *new_argv[4];

	retval = initng_plugin_callers_dump_active_db();

	/* close all fd, exept stdin, stdout, stderr, and that ones that point to our services */
	for (i = 3; i < 1024; i++)
	{
		if (fd_used_by_service(i) == FALSE)
			close(i);
	}

	if (retval == TRUE)
	{
		D_("exec()int initng\n");
		new_argv[0] = i_strdup("/sbin/initng");
		new_argv[1] = i_strdup("--hot_reload");
		new_argv[2] = NULL;

		execve("/sbin/initng", new_argv, environ);
		F_("Failed to reload initng!\n");
	}
	else
	{
		if (retval == FALSE)
			F_("No plugin was willing to dump state\n");
		else
			F_("dump_state failed!\n");
	}
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

	while (!feof(fil))
	{
		active_db_h *new_entry = NULL;
		s_data *d = NULL;
		data_save_struct entry;

		if (!fread(&entry, sizeof(entry), 1, fil))
			continue;

		if (initng_active_db_find_by_name(entry.name))
		{
			W_("Entry exists, won't create it!\n");
			continue;
		}

		/* create a new service entry */
		if (!(new_entry = initng_active_db_new(entry.name)))
		{
			F_("Can't create new active!\n");
			success = FALSE;
			continue;
		}

		/* set current_state */
		new_entry->current_state = initng_active_state_find(entry.state);
		if (!new_entry->current_state)
		{
			F_("Could not find a proper state to set: %s.\n", entry.state);
			success = FALSE;
			continue;
		}

		/* set service stype */
		if (!(new_entry->type = initng_service_type_get_by_name(entry.type)))
		{
			F_("Unknown service type %s.\n", entry.type);
			success = FALSE;
			continue;
		}

		/* set time_current_state */
		memcpy(&new_entry->time_current_state, &entry.time_current_state,
			   sizeof(struct timeval));

		/* walk through all processes */
		{
			int pnr = 0;

			while (entry.process[pnr].ptype[0] && pnr < MAX_PROCESSES)
			{
				process_h *process = NULL;
				ptype_h *pt = NULL;

				while_ptypes(pt)
				{
					if (strcmp(entry.process[pnr].ptype, pt->name) == 0)
						break;
				}

				/* check so it was found */
				if (strcmp(entry.process[pnr].ptype, pt->name) != 0)
				{
					F_("Unknown process type %s\n", entry.process[pnr].ptype);
					pnr++;
					continue;
				}

				/* allocate the process */
				process = initng_process_db_new(pt);
				if (!process)
					continue;

				/* fill the data */
				process->pid = entry.process[pnr].pid;

				{
					pipe_h *op = i_calloc(1, sizeof(pipe_h));

					if (!op)
					{
						free(process);
						continue;
					}

					op->pipe[0] = entry.process[pnr].stdout1;
					op->pipe[1] = entry.process[pnr].stdout2;
					op->dir = BUFFERED_OUT_PIPE;
					op->pipet = PIPE_STDOUT;
					op->targets[0] = 1;
					op->targets[1] = 2;
					add_pipe(op, process);
				}
				process->r_code = entry.process[pnr].rcode;

				/* add this process to the list */
				list_add(&process->list, &new_entry->processes.list);

				D_("Added process type %i to %s\n", process->pt,
				   new_entry->name);

				pnr++;
			}
		}

		{
			int i = 0;

			while (entry.data[i].opt_type)
			{
				d = (s_data *) i_calloc(1, sizeof(s_data));
				d->type = initng_service_data_type_find(entry.data[i].type);
				if (!d->type)
				{
					F_("Did not found %s!\n", entry.data[i].type);
					free(d);
					i++;
					continue;
				}
				switch (d->type->opt_type)
				{
					case STRING:
					case STRINGS:
					case VARIABLE_STRING:
					case VARIABLE_STRINGS:
						d->t.s = i_strdup(entry.data[i].t.s);
						break;
					case INT:
					case VARIABLE_INT:
						d->t.i = entry.data[i].t.i;
						break;
					default:
						break;
				}
				list_add(&d->list, &new_entry->data.head.list);
				i++;
			}
		}

		/* add the new service to the active_db */
		if (initng_active_db_register(new_entry) != TRUE)
		{
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
	if (unlink(filename) != 0)
	{
		W_("Failed removing file %s !!!\n", filename);
		return success;						/* not important */
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
	if (!fil)
	{
		F_("Could not open '%s' for writing\n", filename);
		return FALSE;
	}

	/* walk the active_db */
	while_active_db_safe(current, q)
	{
		if (!current->current_state)
		{
			F_("State is not set, wont save this one!\n");
			continue;
		}

		if (strlen(current->name) >= MAX_SERVICE_NAME_STRING_LEN)
		{
			F_("Service name is to long, it won't fit the fileformat spec! max is %i, won't save this service!\n", MAX_SERVICE_NAME_STRING_LEN);
			success = FALSE;
			continue;
		}

		memset(&entry, 0, sizeof entry);
		strncpy(entry.name, current->name, MAX_SERVICE_NAME_STRING_LEN);
		strncpy(entry.state, current->current_state->state_name, 100);
		if (current->type)
			strncpy(entry.type, current->type->name, 100);
		memcpy(&entry.time_current_state, &current->time_current_state,
			   sizeof(struct timeval));

		/* collect some processes */
		process = NULL;
		pnr = 0;
		while_processes(process, current)
		{
			strncpy(entry.process[pnr].ptype, process->pt->name,
					MAX_PTYPE_STRING_LEN);
			entry.process[pnr].pid = process->pid;

			current_pipe = NULL;
			while_pipes(current_pipe, process)
			{
				entry.process[pnr].stdout1 = current_pipe->pipe[0];
				entry.process[pnr].stdout2 = current_pipe->pipe[1];

				/* TODO, add them all! */
				break;
			}

			entry.process[pnr].rcode = process->r_code;
			pnr++;
			if (pnr >= MAX_PROCESSES)
				break;
		}
		entry.process[pnr].ptype[0] = '\0';

		/* reset data */
		for (i = 0; i < MAX_ENTRYS_FOR_SERVICE; i++)
		{
			entry.data[i].opt_type = 0;
			entry.data[i].type[0] = '\0';
			entry.data[i].t.i = 0;
		}

		i = 0;
		list_for_each_entry(c_d, &current->data.head.list, list)
		{
			if (!c_d->type)
				continue;
			if (!c_d->type->opt_name)
				continue;

			entry.data[i].opt_type = c_d->type->opt_type;
			strncpy(entry.data[i].type, c_d->type->opt_name,
					MAX_TYPE_STRING_LEN);

			/* copy the data */
			switch (c_d->type->opt_type)
			{
				case STRING:
				case STRINGS:
				case VARIABLE_STRING:
				case VARIABLE_STRINGS:
					strncpy(entry.data[i].t.s, c_d->t.s, MAX_DATA_STRING_LEN);
					break;
				case INT:
				case VARIABLE_INT:
					entry.data[i].t.i = c_d->t.i;
					break;
				default:
					break;
			}

			/* save in next data entry row */
			i++;

			/* maximum data entries that can be saved */
			if (i == MAX_ENTRYS_FOR_SERVICE)
			{
				F_("Maximum 20 data entries / service can't be saved!\n");
				success = FALSE;
				break;
			}
		}

		D_("Saving : %s\n", entry.name);
		if (fwrite(&entry, sizeof(entry), 1, fil) != 1)
		{
			F_("failed to write entry '%s': %m\n", entry.name);
			/* TODO: database recovery?? */
			success = FALSE;
			break;
		}
	}

	fclose(fil);
	return success;
}

static int dump_state(void)
{
	const char *file = NULL;

	/* set the correct filename */
	if (g.i_am == I_AM_INIT)
		file = SAVE_FILE;
	else if (g.i_am == I_AM_FAKE_INIT)
		file = SAVE_FILE_FAKE;

	/* if there is a file */
	if (!file)
		return (TRUE);

	return (write_file(file));
}

static int reload_state(void)
{
	struct stat st;
	const char *file = NULL;

	/* set the correct filename */
	if (g.i_am == I_AM_INIT)
		file = SAVE_FILE;
	else if (g.i_am == I_AM_FAKE_INIT)
		file = SAVE_FILE_FAKE;

	/* if there is a file */
	if (!file)
		return (TRUE);

	/* check that file exits */
	if (stat(file, &st) != 0)
	{
		D_("No state file found, passing on reload_state request\n");
		return (FALSE);
	}

	/* return with file */
	return (read_file(file));
}

/* Save a reload file for backup if initng segfaults */
static void save_backup(h_sys_state state)
{
	/* only save when system is getting up */
	if (state == STATE_UP)
	{
		/* save file */
		if (g.i_am == I_AM_INIT)
		{
			write_file(SAVE_FILE);
		}
		else if (g.i_am == I_AM_FAKE_INIT)
		{
			write_file(SAVE_FILE_FAKE);
		}
		return;
	}

	/* if system is stopping, remove the SAVE_FILE */
	if (state == STATE_STOPPING)
	{
		if (g.i_am == I_AM_INIT)
		{
			unlink(SAVE_FILE);
		}
		else if (g.i_am == I_AM_FAKE_INIT)
		{
			unlink(SAVE_FILE_FAKE);
		}
		return;
	}
}

int module_init(int api_version)
{
	D_("module_init(reload);\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	/*    if (g.hot_reload)
	   {
	   if (g.i_am == I_AM_INIT)
	   {
	   read_file(SAVE_FILE);
	   }
	   else
	   {
	   read_file(SAVE_FILE_FAKE);
	   }
	   } */

	initng_plugin_hook_register(&g.SWATCHERS, 90, &save_backup);
	initng_plugin_hook_register(&g.DUMP_ACTIVE_DB, 10, &dump_state);
	initng_plugin_hook_register(&g.RELOAD_ACTIVE_DB, 10, &reload_state);
	initng_command_register(&FAST_RELOAD);
	return (TRUE);
}

void module_unload(void)
{
	D_("module_unload();\n");
	initng_plugin_hook_unregister(&g.SWATCHERS, &save_backup);
	initng_plugin_hook_unregister(&g.DUMP_ACTIVE_DB, &dump_state);
	initng_plugin_hook_unregister(&g.RELOAD_ACTIVE_DB, &reload_state);
	initng_command_unregister(&FAST_RELOAD);
}
