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

#ifndef INITNG_GLOBAL_H
#define INITNG_GLOBAL_H
#include "initng.h"
#include "initng_active_db.h"
#include "initng_service_cache.h"
#include "initng_module.h"
#include "initng_plugin.h"
#include "initng_struct_data.h"
#include "initng_control_command.h"
#include "initng_static_data_id.h"
#include "initng_active_state.h"

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

typedef enum
{
	I_AM_UNKNOWN = 0,
	I_AM_INIT = 1,
	I_AM_FAKE_INIT = 2,
	I_AM_UTILITY = 3,
} h_i_am;

/* The GLOBAL struct */
typedef struct
{
	/* all the databases */
	service_cache_h service_cache;
	active_db_h active_database;
	a_state_h states;
	ptype_h ptypes;
	m_h module_db;
	s_entry option_db;
	s_command command_db;
	stype_h stypes;

	/* global options */
	data_head data;

	/* all hooks to hook at */
	s_call ASTATUS_CHANGE;		/* Calls as soon as the ASTATUS changes */
	s_call PARSERS;				/* Called when a service needs its data */
	s_call ADDITIONAL_PARSE;	/* Called after a service been parsed, and extra parsing may exist */
	s_call SWATCHERS;			/* Called when system state changes */
	s_call FDWATCHERS;			/* Called when initng open file descriptors receive data */
	s_call PIPEWATCHERS;		/* Called when a service has some output, that initng received */
	s_call SIGNAL;				/* Called when initng rescives a signal, like SIGHUP */
	s_call MAIN;				/* Called every main loop */
	s_call A_FORK;				/* Called after a process forks to start */
	s_call HANDLE_KILLED;		/* Called when a process dies */
	s_call COMPENSATE_TIME;		/* Called when initng detects a system time change */
	s_call ERR_MSG;				/* Called when an error message is sent, so all output plug-ins can show it */
	s_call LAUNCH;				/* Called when a process are getting launched */
	s_call DUMP_ACTIVE_DB;		/* Asks for a plugin willing to dump the acytive_db */
	s_call RELOAD_ACTIVE_DB;	/* Asks for a plugin willing to reload the active_db from a dump */
	s_call DEP_ON;				/* Called when a function tries to find out a service dependency */

	/* new ones */
	s_call IS_CHANGE;			/* Called when the rough state of a service changes */
	s_call START_DEP_MET;		/* Called and all this hooks have to return TRUE for launch to start */
	s_call STOP_DEP_MET;		/* Called and all this hooks have to return TRUE for the service to stop */
	s_call UP_MET;				/* Called when a service is trying to set the RUNNING state, is a up test */


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
	h_i_am i_am;
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
void initng_global_new(int argc, char *argv[], char *env[], h_i_am i_am);
void initng_global_free(void);

/* fast macros to set entrys in g */
#define initng_global_set_sleep(sec) { D_("Sleep set: %i seconds.\n", sec); if(g.sleep_seconds==0||sec<g.sleep_seconds) g.sleep_seconds=sec; }

#endif
