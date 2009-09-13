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

#ifndef INITNG_GLOBAL_H
#define INITNG_GLOBAL_H
#include <initng/misc.h>
#include <initng/active_db.h>
#include <initng/module/module.h>
#include <initng/module.h>
#include <initng/data.h>
#include <initng/command.h>
#include <initng/active_state.h>
#include <initng/static/all.h>
#include <initng/event/all.h>

/* what to do when last process stops */
typedef enum
{
	THEN_NONE = 0,
	THEN_QUIT = 1,
	THEN_RESTART = 2,
	THEN_SULOGIN = 3,
	THEN_NEW_INIT = 4,
	THEN_HALT = 5,
	THEN_REBOOT = 6,
	THEN_POWEROFF = 7,
} h_then;

/* The GLOBAL struct */
typedef struct
{
	/* all the databases */
	active_db_h active_db;
	a_state_h states;
	ptype_h ptypes;
	m_h module_db;
	s_entry option_db;
	s_event_type event_db;
	s_command command_db;
	stype_h stypes;

	/* global options */
	data_head data;

	/* global variables */
	h_sys_state sys_state;
	int modules_to_unload;

	/* time counters */
	struct timeval now;
	int sleep_seconds;

	/* Argument data */
	char **Argv;
	int Argc;
	int maxproclen;
	char *Argv0;

	/* system state data */
	int hot_reload;
	char *runlevel;
	char *old_runlevel;
	char *dev_console;
	int when_out;

	/* next alarm */
	time_t next_alarm;

	/* use with THEN_NEW_INIT */
	char **new_init;
	int no_circular;


#ifdef DEBUG
	/* g.verbose_this
	   0 = no verbose
	   1 = all verbose
	   2 = verbose whats in verbose_this
	   3 = verbose all but, that is %this in verbose_this
	 */
	int verbose;
	char *verbose_this[MAX_VERBOSES];
#endif

} s_global;

/* this is defined in initng_global.c */
extern s_global g;

/* functions for initialize and free s_global g */
void initng_config_global_new(int argc, char *argv[], char *env[]);
void initng_config_global_free(void);

/* fast macros to set entrys in g */
#define initng_global_set_sleep(sec) { D_("Sleep set: %i seconds.\n", sec); if(g.sleep_seconds==0||sec<g.sleep_seconds) g.sleep_seconds=sec; }

#endif /* INITNG_GLOBAL_H */
