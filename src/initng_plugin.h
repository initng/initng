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

#ifndef INITNG_PLUGIN
#define INITNG_PLUGIN
#include <stdarg.h>
#include "initng_system_states.h"
#include "initng_list.h"

/* flags for f_module_h.what - correspond to the arguments of select() */
typedef enum
{
	FDW_READ = 1,				/* Want notification when data is ready to be read */
	FDW_WRITE = 2,							/* when data can be written successfully */
	FDW_ERROR = 4,							/* when an exceptional condition occurs */
} e_fdw;

typedef struct ft_module_h f_module_h;
struct ft_module_h
{
	void (*call_module) (f_module_h * module, e_fdw what);
	e_fdw what;
	int fds;
};

typedef union
{
	/* a skeleton, newer use */
	void *pointer;

	/* put all hook functions here */
	int (*status_change) (active_db_h * service);
	service_cache_h *(*parser) (const char *name);
	void (*swatcher) (h_sys_state state);
	int (*buffer_watcher) (active_db_h * service, process_h * process,
						   pipe_h * pi, char *buffer_pos);
	int (*pipe_watcher) (active_db_h * service, process_h * process,
						 pipe_h * pi);
	int (*launch) (active_db_h * service, process_h * process,
				   const char *exec_name);
	int (*af_launcher) (active_db_h * service, process_h * process);
	int (*handle_killed) (active_db_h * service, process_h * process);
	void (*main) (void);
	void (*compensate_time) (int seconds);
	int (*err) (e_mt mt, const char *file, const char *func, int line,
				const char *format, va_list ap);
	int (*dep_on_check) (active_db_h * service, active_db_h * check);
	void (*signal_hook) (int signal);
	f_module_h *fdh;
	int (*additional_parse) (service_cache_h * service);
	int (*dump_active_db) (void);
	int (*reload_active_db) (void);
	int (*start_dep_met) (active_db_h * service);
	int (*stop_dep_met) (active_db_h * service);
	int (*up_met) (active_db_h * service);
	active_db_h *(*new_active) (const char *name);
} uc __attribute__ ((__transparent_union__));


typedef struct s_callers_s s_call;
struct s_callers_s
{
	char *from_file;
	uc c;
	int order;
	struct list_head list;
};

#define while_list(current, call) list_for_each_entry_prev(current, &(call)->list, list)
#define while_list_safe(current, call, safe) list_for_each_entry_prev_safe(current, safe, &(call)->list, list)


#endif
