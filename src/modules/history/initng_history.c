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

#include <sys/types.h>		/* time_t */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>		/* printf() */
#include <assert.h>
#include <errno.h>

#include "../ngc4/initng_ngc4.h"
#include "initng_history.h"

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { "ngc4", NULL },
	.init = &module_init,
	.unload = &module_unload
};

int history_records = 0;

history_h history_db;

static void cmd_history(char *arg, s_payload * payload)
{
	int i = 0;
	history_h *current = NULL;

	/* allocate space for payload */
	payload->p = initng_toolbox_calloc(HISTORY + 1, sizeof(active_row));

	while_history_db_prev(current) {
		active_row *row = payload->p + sizeof(active_row [i]);

		row->dt = ACTIVE_ROW;

		/* if action is not set, it is probably a string logged in
		 * this db */
		if (!current->action)
			continue;

		if (arg && strlen(arg) > 1 && !((current->name &&
						 strcmp(current->name,
							arg) == 0)
						|| (current->service
						    && current->service->name
						    && strcmp(current->service->
							      name,
							      arg) == 0))) {
			continue;
		}

		if (current->data)
			printf("%s\n", current->data);

		if (current->action)
			strncpy(row->state, current->action->name, 100);
		else
			row->state[0] = '\0';

		if (current->service && current->service->type &&
		    current->service->type->name) {
			strncpy(row->type, current->service->type->name, 100);
		} else {
			row->type[0] = '\0';
		}

		memcpy(&row->time_set, &current->time, sizeof(struct timeval));

		if (current->name)
			strncpy(row->name, current->name, 100);
		else if (current->service && current->service->name)
			strncpy(row->name, current->service->name, 100);
		else
			row->name[0] = '\0';

		/* set the rought state */
		row->is = current->action->is;

		/* increase the number of entrys to send */
		i++;
	}

	payload->s = sizeof(active_row [i]);
}

s_command HISTORYS = {
	.id = 'L',
	.long_id = "show_history",
	.com_type = PAYLOAD_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = USES_OPT,
	.u = {(void *)&cmd_history},
	.description = "Print out history_db."
};

/* This is the maxumum length of a row with ngc -l */
#define LOG_ROW_LEN 70
/* If the row got a space after this number of chars, make a newline to
 * prevent a word breakage */
#define LOG_ROW_LEN_BREAK_ON_SPACE 60
#define NAME_SPACER "20"

static char *cmd_log(char *arg)
{
	char *string = NULL;
	char *name = NULL;
	int only_output = FALSE;
	history_h *current = NULL;
	time_t last = 0;

	/* reset arg, if strlen is short */
	if (arg) {
		if (strlen(arg) < 1)
			arg = NULL;
		else if (strcmp(arg, "output") == 0)
			only_output = TRUE;
	}

	initng_string_mprintf(&string, " %" NAME_SPACER "s : STATUS\n", "SERVICE");
	initng_string_mprintf(&string, " ---------------------------------------"
		"---------------\n");

	while_history_db_prev(current) {
		/* if only_output is set, it have to be an output to
		 * continue */
		if (only_output && !current->data)
			continue;

		/* if there is an argument, it have to match the service */
		if (arg && !((current->name && strcmp(current->name, arg) == 0)
			     || (current->service && current->service->name
				 && strcmp(current->service->name,
					   arg) == 0))) {
			continue;
		}

		/* try to set the service name, from any source found */
		if (current->name)
			name = current->name;
		else if (current->service && current->service->name)
			name = current->service->name;
		else
			name = NULL;

		if (last != current->time.tv_sec) {
			/* print a nice service change status entry */
			char *c = ctime(&current->time.tv_sec);

			initng_string_mprintf(&string, "\n %s", c);
			initng_string_mprintf(&string, " --------------------------------"
				"----------------------\n");

			last = current->time.tv_sec;
		}

		/* if the log entry contains output data ... */
		if (current->data) {
			char *tmp = current->data;
			char buf[LOG_ROW_LEN + 1];

			while (tmp) {
				/* Variable that contains the no of chars to
				 * next newline */
				int i = 0;

				/* skip idention from data source */
				while (tmp[0] == ' ' || tmp[0] == '\t' ||
				       tmp[0] == '\n') {
					tmp++;
				}

				/* if no more left, break */
				if (!tmp[i])
					break;

				/* cont chars to newline */
				while (tmp[i] && tmp[i] != '\n' &&
				       i < LOG_ROW_LEN) {
					/* This rules will decrease the chanse
					 * that a word is breaked */
					if (i > LOG_ROW_LEN_BREAK_ON_SPACE &&
					    (tmp[i] == ' ' || tmp[i] == '\t'))
						break;
					i++;
				}

				/* fill with that row */
				strncpy(buf, tmp, i);
				buf[i] = '\0';

				/* send that to client */
				initng_string_mprintf(&string, " %" NAME_SPACER "s : %s\n",
					name, buf);

				/* where to start next */
				tmp = &tmp[i];
			}
		} else {

			/* only print important state changes */
			switch (current->action->is) {
			default:
				break;
			case IS_UP:
			case IS_DOWN:
			case IS_FAILED:
				/* if there has gone some seconds sence last
				 * change, print that */
				if (current->duration > 0) {
					initng_string_mprintf(&string, " %" NAME_SPACER
						"s | %s (after %i seconds)\n",
						name,
						current->action->name,
						(int)current->duration);
				} else {
					initng_string_mprintf(&string, " %" NAME_SPACER
						"s | %s\n", name,
						current->action->name);
				}
				break;
			}
		}

	}

	return string;
}

s_command LOG = {
	.id = 'l',
	.long_id = "log",
	.com_type = STRING_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = USES_OPT,
	.u = {(void *)&cmd_log},
	.description = "Print out log."
};

static void history_db_compensate_time(s_event * event)
{
	time_t *skew;
	history_h *current = NULL;

	assert(event->event_type == &EVENT_COMPENSATE_TIME);

	skew = event->data;

	D_("history_db_compensate_time(%i);\n", (int)(*skew));

	while_history_db(current) {
		current->time.tv_sec += *skew;
	}
}

static void history_db_clear_service(active_db_h * service)
{
	history_h *current = NULL;

	D_("history_db_clear_service(%s);\n", service->name);

	while_history_db(current) {
		if (current->service == service) {
			current->service = NULL;
			current->name = initng_toolbox_strdup(service->name);
			break;
		}
	}
}

/* clear history */
static void history_free_all(void)
{
	history_h *current, *safe = NULL;

	/* while there is a history db */
	while_history_db_safe(current, safe) {
		/* free history name */
		free(current->name);

		/* free data if any */
		free(current->data);

		/* remove from history_db list */
		initng_list_del(&current->list);

		/* free history entry */
		free(current);
		current = NULL;
	}
}

static int add_hist(history_h * hist)
{
	/* add struct */
	initng_list_add(&hist->list, &history_db.list);

	/* check length of history_db, and purge it on the end! */
	history_records++;

	/* If maximum entrys are reached */
	if (history_records > HISTORY) {
		list_t *last = history_db.list.prev;
		history_h *entry = initng_list_entry(last, history_h, list);

		/* if we got anything */
		if (!entry) {
			F_("Unable to free last histroty entry!, cant add "
			   "more.\n");
			return FALSE;
		}

		/* free the name */
		free(entry->name);

		/* free any data that might be in this history entry */
		free(entry->data);

		/* delete this from the historylist */
		initng_list_del(last);

		/* free the struct */
		free(entry);
		entry = NULL;

		/* count down the number of records */
		history_records--;
	}

	/* leave */
	return TRUE;
}

/* add values to history database */
static void history_add_values(s_event * event)
{
	active_db_h *service;
	history_h *tmp_e = NULL;	/* temporary pointer to insert data
					 * and add to db */

	assert(event->event_type == &EVENT_STATE_CHANGE);
	assert(event->data);

	service = event->data;

	assert(service->name);

	/* Don't bother adding */
	if (!service->current_state)
		return;

	/*if (!service->current_state->state_name);
	   return(TRUE); */

	D_("adding: %s.\n", service->name);

	/* allocate space for data */
	if (!(tmp_e = (history_h *)
	      initng_toolbox_calloc(1, sizeof(history_h)))) {
		F_("Out of memory.\n");
		return;
	}

	/* set data in struct */
	tmp_e->service = service;
	tmp_e->name = NULL;
	memcpy(&tmp_e->time, &service->time_current_state,
	       sizeof(struct timeval));
	tmp_e->action = service->current_state;

	/* set duration if possible */
	if (service->last_state && service->time_last_state.tv_sec > 1) {
		tmp_e->duration = difftime(service->time_current_state.tv_sec,
					   service->time_last_state.tv_sec);
	}

	/*D_("history_add_values() service : %s, name: %s, action: %s\n",
	 *   service->name, NULL, service->current_state->state_name); */

	add_hist(tmp_e);

	/* if service is status freeing, clear the pointers in history db */
	if (IS_MARK(service, &FREEING)) {
		history_db_clear_service(service);
	}

	/* leave */
}

static void fetch_output(s_event * event)
{
	s_event_buffer_watcher_data *data;
	history_h *tmp_e = NULL;

	S_;
	assert(event->event_type == &EVENT_BUFFER_WATCHER);
	assert(event->data);

	data = event->data;
	assert(data->buffer_pos);

	/* allocate space for data */
	if (!(tmp_e = (history_h *)
	      initng_toolbox_calloc(1, sizeof(history_h)))) {
		F_("Out of memory.\n");
		return;
	}

	/* set data in struct */
	tmp_e->service = data->service;
	tmp_e->name = NULL;
	initng_gettime_monotonic(&tmp_e->time);
	tmp_e->data = initng_toolbox_strdup(data->buffer_pos);
	tmp_e->action = NULL;

	/* add to history struct */
	add_hist(tmp_e);
}

int module_init(void)
{
	initng_list_init(&history_db.list);

	initng_command_register(&HISTORYS);
	initng_command_register(&LOG);
	initng_event_hook_register(&EVENT_STATE_CHANGE, &history_add_values);
	initng_event_hook_register(&EVENT_COMPENSATE_TIME,
				   &history_db_compensate_time);
	initng_event_hook_register(&EVENT_BUFFER_WATCHER, &fetch_output);

	return TRUE;
}

void module_unload(void)
{
	initng_command_unregister(&HISTORYS);
	initng_command_unregister(&LOG);
	history_free_all();
	initng_event_hook_unregister(&EVENT_STATE_CHANGE, &history_add_values);
	initng_event_hook_unregister(&EVENT_COMPENSATE_TIME,
				     &history_db_compensate_time);
	initng_event_hook_unregister(&EVENT_BUFFER_WATCHER, &fetch_output);
}
