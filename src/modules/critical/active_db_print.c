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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "active_db_print.h"

#define IS_PRINTABLE(x) (x >= 32 || x == '\n' || x == '\t' || x == '\r')

static void print_string_value(char *string);

/* P R I N T   S E R V I C E */

/* there may be unprintable characters in string - should escape them when printing */
static void print_string_value(char *string)
{
	int i;

	for (i = 0; string[i] != 0; i++) {
		if (IS_PRINTABLE(string[i]))
			printf("%c", string[i]);
		else
			printf("^%c", string[i] ^ 0x40);
	}
}

static void print_sdata(s_data * tmp)
{
	if (!tmp->type)
		return;

	switch (tmp->type->type) {
	case STRING:
	case STRINGS:
		if (!tmp->t.s) {
			F_("empty value!\n#");
			return;
		}

		printf("\t %10s            = \"", tmp->type->name);

		print_string_value(tmp->t.s);
		printf("\"\n#");
		return;

	case VARIABLE_STRING:
	case VARIABLE_STRINGS:
		if (!tmp->t.s) {
			F_("empty value!\n#");
			return;
		}

		if (tmp->vn) {
			printf("\t %10s %-10s = \"", tmp->type->name, tmp->vn);
		} else {
			printf("\t %10s %-10s = \"", tmp->type->name, "ERROR");
		}

		print_string_value(tmp->t.s);
		printf("\"\n#");
		return;

	case INT:
		printf("\t %10s            = \"%i\"\n#", tmp->type->name,
		       tmp->t.i);
		return;

	case VARIABLE_INT:
		printf("\t %10s %-10s = \"%i\"\n#", tmp->type->name, tmp->vn,
		       tmp->t.i);
		return;

	case SET:
		printf("\t %10s            = TRUE\n#", tmp->type->name);
		return;

	case VARIABLE_SET:
		printf("\t %10s %-10s = TRUE\n#", tmp->type->name, tmp->vn);
		return;

	case ALIAS:
		printf("\t ALIAS %10s\n#", tmp->type->name);
		return;

	default:
		return;
	}
}

static void active_db_print_process(process_h * p)
{
	pipe_h *current_pipe = NULL;

	assert(p);

	if (p->pst == P_FREE)
		printf("\t DEAD Process: type %s\n#", p->pt->name);
	else if (p->pst == P_ACTIVE)
		printf("\t Process: type %s\n#", p->pt->name);

	if (p->pid > 0)
		printf("\t\tPid: %i\n#", p->pid);

	if (p->r_code > 0)
		printf("\t\tSIGNALS:\n#"
		       "\t\tWEXITSTATUS %i\n#"
		       "\t\tWIFEXITED %i\n#"
		       "\t\tWIFSIGNALED %i\n#" "\t\tWTERMSIG %i\n#"
#ifdef WCOREDUMP
		       "\t\tWCOREDUMP %i\n#"
#endif
		       "\t\tWIFSTOPPED %i\n#"
		       "\t\tWSTOPSIG %i\n#"
		       "\n#",
		       WEXITSTATUS(p->r_code), WIFEXITED(p->r_code),
		       WIFSIGNALED(p->r_code), WTERMSIG(p->r_code),
#ifdef WCOREDUMP
		       WCOREDUMP(p->r_code),
#endif
		       WIFSTOPPED(p->r_code), WSTOPSIG(p->r_code));

	if (!initng_list_isempty(&p->pipes.list)) {
		printf("\t\tPIPES:\n#");
		while_pipes(current_pipe, p) {
			int i;

			switch (current_pipe->dir) {
			case IN_PIPE:
				printf("\t\t INPUT_PIPE read: %i, "
				       "write: %i remote:",
				       current_pipe->pipe[0],
				       current_pipe->pipe[1]);
				break;

			case OUT_PIPE:
				printf("\t\t OUTPUT_PIPE read: %i, "
				       "write: %i remote:",
				       current_pipe->pipe[1],
				       current_pipe->pipe[0]);
				break;

			case BUFFERED_OUT_PIPE:
				printf("\t\t BUFFERED_OUTPUT_PIPE "
				       "read: %i, write: %i remote:",
				       current_pipe->pipe[1],
				       current_pipe->pipe[0]);
				break;

			default:
				continue;
			}

			for (i = 0; current_pipe->targets[i] > 0 &&
			     i < MAX_TARGETS; i++)
				printf(" %i", current_pipe->targets[i]);

			printf("\n#");
			if (current_pipe->buffer &&
			    current_pipe->buffer_allocated > 0) {
				printf("\t\tBuffer (%i): SE BELOW\n"
				       "##########  BUFFER  ##########\n%s\n"
				       "##############################\n#",
				       current_pipe->buffer_allocated,
				       current_pipe->buffer);
			}
		}
	}

}

void active_db_print(active_db_h * s)
{
	/*data path */
	s_data *tmp = NULL;
	process_h *process = NULL;

	assert(s);
	assert(s->name);

	struct timeval now;
	printf("\n\n\n\n\n##################################################"
	       "##############################\n#SERVICE DATA DUMP:\n#");
	printf("\n# %s  \"%s", s->type->name, s->name);

	if (s->current_state && s->current_state->name) {
		printf("\"  status  \"%s\"\n#", s->current_state->name);
	} else {
		printf("\"\n#");
	}

	initng_gettime_monotonic(&now);

	printf("\t TIMES:\n#\t last_rought: %ims\n#\t last_state: %ims\n#"
	       "\t current_state: %ims\n#",
	       MS_DIFF(now, s->last_rought_time),
	       MS_DIFF(now, s->time_last_state),
	       MS_DIFF(now, s->time_current_state));

	/* print processes if any */
	printf(" \tPROCCESSES:\n#");
	if (!initng_list_isempty(&s->processes.list)) {
		while_processes(process, s) {
			active_db_print_process(process);
		}
	}

	printf(" \tVARIABLES:\n#");
	if (!initng_list_isempty(&s->data.head.list)) {
		initng_list_foreach(tmp, &(s->data.head.list), list) {
			print_sdata(tmp);
		}
	}

	printf("###########################################################"
	       "#####################\n\n\n");
}
