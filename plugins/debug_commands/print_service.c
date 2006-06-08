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

#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include <initng_toolbox.h>
#include <initng_global.h>
#include <initng_service_cache.h>
#include <initng_active_db.h>
#include <initng_active_state.h>
#include <initng_string_tools.h>

#include "print_service.h"

INITNG_PLUGIN_MACRO;

#define IS_PRINTABLE(x) (x >= 32 || x == '\n' || x == '\t' || x == '\r')

static void service_db_print_u(service_cache_h * s, char **string);
static void active_db_print_u(active_db_h * s, char **string);
static void print_string_value(char *string, char **to);

/* P R I N T   S E R V I C E */

/* some stdout */
char *service_db_print(service_cache_h * s)
{
	char *string = NULL;

	service_db_print_u(s, &string);
	return (string);
}

char *active_db_print(active_db_h * s)
{
	char *string = NULL;

	active_db_print_u(s, &string);
	return (string);
}

/* there may be unprintable characters in string - should escape them when printing */
static void print_string_value(char *string, char **to)
{
	int i;

	for (i = 0; string[i] != 0; i++)
	{
		if (IS_PRINTABLE(string[i]))
			mprintf(to, "%c", string[i]);
		else
			mprintf(to, "^%c", string[i] ^ 0x40);
	}
}

static void print_sdata(s_data * tmp, char **string)
{
	if (!tmp->type)
		return;
	switch (tmp->type->opt_type)
	{
		case STRING:
		case STRINGS:
			if (!tmp->t.s)
			{
				F_("empty value!\n");
				return;
			}
			mprintf(string, " @@ %-24s : \"", tmp->type->opt_name);

			print_string_value(tmp->t.s, string);
			mprintf(string, "\"\n");
			return;
		case VARIABLE_STRING:
		case VARIABLE_STRINGS:
			if (!tmp->t.s)
			{
				F_("empty value!\n");
				return;
			}
			if (tmp->vn)
				mprintf(string, " @@ %-13s %-10s : \"", tmp->type->opt_name,
						tmp->vn);
			else
				mprintf(string, " @@ %-13s %-10s : \"", tmp->type->opt_name,
						tmp->vn);

			print_string_value(tmp->t.s, string);
			mprintf(string, "\"\n");
			return;
		case INT:
			mprintf(string, " @@ %-24s : \"%i\"\n", tmp->type->opt_name,
					tmp->t.i);
			return;
		case VARIABLE_INT:
			mprintf(string, " @@ %-13s %-10s : \"%i\"\n", tmp->type->opt_name,
					tmp->vn, tmp->t.i);
			return;
		case SET:
			mprintf(string, " @@ %-24s\n", tmp->type->opt_name);
			return;
		case VARIABLE_SET:
			mprintf(string, " @@ %-13s %-10s\n", tmp->type->opt_name,
					tmp->vn);
			return;
		case ALIAS:
			mprintf(string, " @@ ALIAS %-24s\n", tmp->type->opt_name);
			return;
		default:
			return;
	}

}



static void service_db_print_u(service_cache_h * s, char **string)
{
	/*data path */
	s_data *tmp = NULL;

	assert(s);
	assert(s->name);

	mprintf(string, "\n# service_cache_entry: %s  \"%s", s->type->name,
			s->name);

	if (s->father_name)
		mprintf(string, " : %s", s->father_name);

	mprintf(string, "\"\n");

	mprintf(string, "    ---------------------------------- \n");

	list_for_each_entry(tmp, &s->data.head.list, list)
	{
		print_sdata(tmp, string);
	}

	/* if father with data exits, print it */
	if (s->father)
	{
		D_("FATHER FOUND %s\n", s->name);
		service_db_print_u(s->father, string);
	}
	else
		D_("FATHER NOT FOUND %s\n", s->name);
}


/* Walk through every service and print it all */
char *service_db_print_all(void)
{
	char *string = NULL;
	service_cache_h *current = NULL;

	mprintf(&string, "Full service print:\n");

	D_("print_all():\n");

	while_service_cache(current)
	{
		D_("printing %s ...\n", current->name);
		service_db_print_u(current, &string);
	}

	return (string);
}

static void active_db_print_process(process_h * p, char **string)
{
	pipe_h *current_pipe = NULL;

	assert(p);
	if (p->pst == P_FREE)
		mprintf(string, "\tOLD    Process: type %s\n", p->pt->name);
	if (p->pst == P_ACTIVE)
		mprintf(string, "\tACTIVE Process: type %s\n", p->pt->name);

	if (p->pid > 0)
		mprintf(string, "\t\tPid: %i\n", p->pid);

	if (p->r_code > 0)
		mprintf(string, "\t\tSIGNALS:\n"
				"\t\tWEXITSTATUS %i\n"
				"\t\tWIFEXITED %i\n"
				"\t\tWIFSIGNALED %i\n" "\t\tWTERMSIG %i\n"
#ifdef WCOREDUMP
				"\t\tWCOREDUMP %i\n"
#endif
				"\t\tWIFSTOPPED %i\n"
				"\t\tWSTOPSIG %i\n"
				"\n",
				WEXITSTATUS(p->r_code),
				WIFEXITED(p->r_code),
				WIFSIGNALED(p->r_code), WTERMSIG(p->r_code),
#ifdef WCOREDUMP
				WCOREDUMP(p->r_code),
#endif
				WIFSTOPPED(p->r_code), WSTOPSIG(p->r_code));

	while_pipes(current_pipe, p)
	{
		int i;

		switch (current_pipe->dir)
		{
			case IN_PIPE:
				mprintf(string, "\t\tINPUT_PIPE read: %i, write: %i remote:",
						current_pipe->pipe[0], current_pipe->pipe[1]);
				break;
			case OUT_PIPE:
				mprintf(string, "\t\tOUTPUT_PIPE read: %i, write: %i remote:",
						current_pipe->pipe[1], current_pipe->pipe[0]);
				break;
			case BUFFERED_OUT_PIPE:
				mprintf(string,
						"\t\tBUFFERED_OUTPUT_PIPE read: %i, write: %i remote:",
						current_pipe->pipe[1], current_pipe->pipe[0]);
				break;
			default:
				continue;
		}

		for (i = 0; current_pipe->targets[i] > 0 && i < MAX_TARGETS; i++)
			mprintf(string, " %i", current_pipe->targets[i]);

		mprintf(string, "\n");
		if (current_pipe->buffer && current_pipe->buffer_allocated > 0)
		{
			mprintf(string,
					"\t\tBuffer (%i): \n##########  BUFFER  ##########\n%s\n##############################\n",
					current_pipe->buffer_allocated, current_pipe->buffer);
		}
	}


}



static void active_db_print_u(active_db_h * s, char **string)
{
	/*data path */
	s_data *tmp = NULL;
	process_h *process = NULL;

	assert(s);
	assert(s->name);


	struct timeval now;

	mprintf(string, "\n#  %s  \"%s", s->type->name, s->name);
	if (s->from_service)
	{
		mprintf(string, " :: %s", s->from_service->name);
		if (s->from_service->father_name)
		{
			mprintf(string, " : %s", s->from_service->father_name);
		}
	}
	mprintf(string, "\"\n");

	if (s->current_state && s->current_state->state_name)
	{
		mprintf(string, "\tactive status: %s\n",
				s->current_state->state_name);
	}

	gettimeofday(&now, NULL);

	mprintf(string,
			"\tlast_rought_time: %ims time_last_state: %ims time_current_state: %ims\n",
			MS_DIFF(now, s->last_rought_time), MS_DIFF(now,
													   s->
													   time_last_state),
			MS_DIFF(now, s->time_current_state));

	/* print processes if any */

	while_processes(process, s)
	{
		active_db_print_process(process, string);
	}






	mprintf(string, "\tactive_db opts:\n");

	list_for_each_entry(tmp, &(s->data.head.list), list)
	{
		print_sdata(tmp, string);
	}

	if (s->from_service)
	{
		mprintf(string, "\tservice_cache opts:\n");
		tmp = NULL;
		list_for_each_entry(tmp, &(s->from_service->data.head.list), list)
		{
			print_sdata(tmp, string);
		}
	}
}

/* Walk through every service and print it all */
char *active_db_print_all(void)
{
	char *string = NULL;
	active_db_h *apt = NULL;

	D_("print_all():\n");

	while_active_db(apt)
	{
		active_db_print_u(apt, &string);
	}

	return (string);
}
