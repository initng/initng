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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <assert.h>

#include <sys/time.h>
#include <sys/resource.h>

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = NULL,
	.init = &module_init,
	.unload = &module_unload
};


/* stolen from man setrlimit */
s_entry RLIMIT_AS_SOFT = {
	.name = "rlimit_as_soft",
	.description = "The maximum size of process virtual memory, soft "
	    "limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_AS_HARD = {
	.name = "rlimit_as_hard",
	.description = "The maximum size of process virtual memory, hard "
	    "limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_CORE_SOFT = {
	.name = "rlimit_core_soft",
	.description = "The maximum size of the core file, soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_CORE_HARD = {
	.name = "rlimit_core_hard",
	.description = "The maximum size of the core file, hard limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_CPU_SOFT = {
	.name = "rlimit_cpu_soft",
	.description = "The maximum of cputime, in seconds processes can "
	    "use, soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_CPU_HARD = {
	.name = "rlimit_cpu_hard",
	.description = "The maximum of cputime, in seconds processes can "
	    "use, hard limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_DATA_SOFT = {
	.name = "rlimit_data_soft",
	.description = "The maximum size of the process uninitalized data, "
	    "soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_DATA_HARD = {
	.name = "rlimit_data_hard",
	.description = "The maximum size of the process uninitalized data, "
	    "hard limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_FSIZE_SOFT = {
	.name = "rlimit_fsize_soft",
	.description = "The maximum filesize of a file the process can "
	    "create, soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_FSIZE_HARD = {
	.name = "rlimit_fsize_hard",
	.description = "The maximum filesize of a file the process can "
	    "create, hard limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_MEMLOCK_SOFT = {
	.name = "rlimit_memlock_soft",
	.description = "The maximum amount of memory, the process can lock, "
	    "soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_MEMLOCK_HARD = {
	.name = "rlimit_memlock_hard",
	.description = "The maximum amount of memory, the process can lock, "
	    "hard limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_NOFILE_SOFT = {
	.name = "rlimit_nofile_soft",
	.description = "The maximum number of files the process can open, at "
	    "the same time, soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_NOFILE_HARD = {
	.name = "rlimit_nofile_hard",
	.description = "The maximum number of files the process can open, at "
	    "the same time, hard limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_NPROC_SOFT = {
	.name = "rlimit_nproc_soft",
	.description = "The maximum number of processes that can be created, "
	    "soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_NPROC_HARD = {
	.name = "rlimit_nproc_hard",
	.description = "The maximum number of processes that can be created, "
	    "hard limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_RSS_SOFT = {
	.name = "rlimit_rss_soft",
	.description = "The maximum no of virtual pages in ram, soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_RSS_HARD = {
	.name = "rlimit_rss_hard",
	.description = "The maximum no of virtual pages in ram, hard limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_SIGPENDING_SOFT = {
	.name = "rlimit_sigpending_soft",
	.description = "The maximum amount of signals that can be queued, "
	    "soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_SIGPENDING_HARD = {
	.name = "rlimit_sigpending_hard",
	.description = "The maximum amount of signals that can be queued, "
	    "soft limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_STACK_SOFT = {
	.name = "rlimit_stack_soft",
	.description = "The maximum size of the process stack, hard limit.",
	.type = INT,
	.ot = NULL,
};

s_entry RLIMIT_STACK_HARD = {
	.name = "rlimit_stack_hard",
	.description = "The maximum size of the process stack, soft limit.",
	.type = INT,
	.ot = NULL,
};

/* fetch the correct error string */
static const char *err_desc(void)
{
	switch (errno) {
	case EFAULT:
		return "Rlim prints outside the accessible address space.\n";

	case EINVAL:
		return "Resource is not valid.\n";

	case EPERM:
		return "Unprivileged process tried to set rlimit.\n";

	default:
		return NULL;
	}
}

/* this function set rlimit if it should, w-o overwriting old values. */
static int set_limit(s_entry * soft, s_entry * hard, active_db_h * service,
		     int ltype, int times)
{
	int si = FALSE;
	int sh = FALSE;
	struct rlimit l;

	/* check if the limits are set */
	si = is(soft, service);
	sh = is(hard, service);

	/* make sure any is set */
	if (si == FALSE && sh == FALSE)
		return (0);

	/* get the current limit data */
	if (getrlimit(ltype, &l) != 0) {
		F_("getrlimit failed!, service %s, limit type %i: %s\n",
		   service->name, ltype, err_desc());
		return -1;
	}

	D_("current: soft: %i, hard: %i\n", l.rlim_cur, l.rlim_max);
	/* if soft limit is set, get it */
	if (si)
		l.rlim_cur = (get_int(soft, service) * times);

	/* if hard limit is set, get it */
	if (sh)
		l.rlim_max = (get_int(hard, service) * times);

	/* make sure hard is at least same big, but hopefully bigger. */
	if (l.rlim_cur > l.rlim_max)
		l.rlim_max = l.rlim_cur;

	D_("now: soft: %i, hard: %i\n", l.rlim_cur, l.rlim_max);

	/* set the limit and return status */
	if (setrlimit(ltype, &l) != 0) {
		F_("setrlimit failed, service: %s, limit type %i: %s\n",
		   service->name, ltype, err_desc());
		return -1;
	}

	return 0;
}

static void do_limit(s_event * event)
{
	s_event_after_fork_data *data;

	int ret = 0;

	assert(event->event_type == &EVENT_AFTER_FORK);
	assert(event->data);

	data = event->data;

	assert(data->service);
	assert(data->service->name);
	assert(data->process);

	D_("do_limit!\n");

	/* Handle RLIMIT_AS */
	ret += set_limit(&RLIMIT_AS_SOFT, &RLIMIT_AS_HARD, data->service,
			 RLIMIT_AS, 1024);

	/* Handle RLIMIT_CORE */
	ret += set_limit(&RLIMIT_CORE_SOFT, &RLIMIT_CORE_HARD, data->service,
			 RLIMIT_CORE, 1024);

	/* Handle RLIMIT_CPU */
	ret += set_limit(&RLIMIT_CPU_SOFT, &RLIMIT_CPU_HARD, data->service,
			 RLIMIT_CPU, 1);

	/* Handle RLIMIT_DATA */
	ret += set_limit(&RLIMIT_DATA_SOFT, &RLIMIT_DATA_HARD, data->service,
			 RLIMIT_DATA, 1024);

	/* Handle RLIMIT_FSIZE */
	ret += set_limit(&RLIMIT_FSIZE_SOFT, &RLIMIT_FSIZE_HARD,
			 data->service, RLIMIT_FSIZE, 1024);

	/* Handle RLIMIT_MEMLOCK */
	ret += set_limit(&RLIMIT_MEMLOCK_SOFT, &RLIMIT_MEMLOCK_HARD,
			 data->service, RLIMIT_MEMLOCK, 1024);

	/* Handle RLIMIT_NOFILE */
	ret += set_limit(&RLIMIT_NOFILE_SOFT, &RLIMIT_NOFILE_HARD,
			 data->service, RLIMIT_NOFILE, 1);

	/* Handle RLIMIT_NPROC */
	ret += set_limit(&RLIMIT_NPROC_SOFT, &RLIMIT_NPROC_HARD,
			 data->service, RLIMIT_NPROC, 1);

	/* Handle RLIMIT_RSS */
	ret += set_limit(&RLIMIT_RSS_SOFT, &RLIMIT_RSS_HARD, data->service,
			 RLIMIT_RSS, 1024);

#ifdef RLIMIT_SIGPENDING
	/* for some reason, this seems missing on some systems */
	/* Handle RLIMIT_SIGPENDING */
	ret += set_limit(&RLIMIT_SIGPENDING_SOFT, &RLIMIT_SIGPENDING_HARD,
			 data->service, RLIMIT_SIGPENDING, 1);
#endif

	/* Handle RLIMIT_STACK */
	ret += set_limit(&RLIMIT_STACK_SOFT, &RLIMIT_STACK_HARD,
			 data->service, RLIMIT_STACK, 1024);

	/* make sure every rlimit suceeded */
	if (ret != 0)
		event->status = FAILED;
}

int module_init(void)
{
	/* Add all options to initng */
	initng_service_data_type_register(&RLIMIT_AS_SOFT);
	initng_service_data_type_register(&RLIMIT_AS_HARD);
	initng_service_data_type_register(&RLIMIT_CORE_SOFT);
	initng_service_data_type_register(&RLIMIT_CORE_HARD);
	initng_service_data_type_register(&RLIMIT_CPU_SOFT);
	initng_service_data_type_register(&RLIMIT_CPU_HARD);
	initng_service_data_type_register(&RLIMIT_DATA_SOFT);
	initng_service_data_type_register(&RLIMIT_DATA_HARD);
	initng_service_data_type_register(&RLIMIT_FSIZE_SOFT);
	initng_service_data_type_register(&RLIMIT_FSIZE_HARD);
	initng_service_data_type_register(&RLIMIT_MEMLOCK_SOFT);
	initng_service_data_type_register(&RLIMIT_MEMLOCK_HARD);
	initng_service_data_type_register(&RLIMIT_NOFILE_SOFT);
	initng_service_data_type_register(&RLIMIT_NOFILE_HARD);
	initng_service_data_type_register(&RLIMIT_NPROC_SOFT);
	initng_service_data_type_register(&RLIMIT_NPROC_HARD);
	initng_service_data_type_register(&RLIMIT_RSS_SOFT);
	initng_service_data_type_register(&RLIMIT_RSS_HARD);
	initng_service_data_type_register(&RLIMIT_SIGPENDING_SOFT);
	initng_service_data_type_register(&RLIMIT_SIGPENDING_HARD);
	initng_service_data_type_register(&RLIMIT_STACK_SOFT);
	initng_service_data_type_register(&RLIMIT_STACK_HARD);

	/* add the after fork function hook */
	initng_event_hook_register(&EVENT_AFTER_FORK, &do_limit);

	/* always return happily */
	return TRUE;
}

void module_unload(void)
{
	/* remove the hook */
	initng_event_hook_unregister(&EVENT_AFTER_FORK, &do_limit);

	/* Del all options to initng */
	initng_service_data_type_unregister(&RLIMIT_AS_SOFT);
	initng_service_data_type_unregister(&RLIMIT_AS_HARD);
	initng_service_data_type_unregister(&RLIMIT_CORE_SOFT);
	initng_service_data_type_unregister(&RLIMIT_CORE_HARD);
	initng_service_data_type_unregister(&RLIMIT_CPU_SOFT);
	initng_service_data_type_unregister(&RLIMIT_CPU_HARD);
	initng_service_data_type_unregister(&RLIMIT_DATA_SOFT);
	initng_service_data_type_unregister(&RLIMIT_DATA_HARD);
	initng_service_data_type_unregister(&RLIMIT_FSIZE_SOFT);
	initng_service_data_type_unregister(&RLIMIT_FSIZE_HARD);
	initng_service_data_type_unregister(&RLIMIT_MEMLOCK_SOFT);
	initng_service_data_type_unregister(&RLIMIT_MEMLOCK_HARD);
	initng_service_data_type_unregister(&RLIMIT_NOFILE_SOFT);
	initng_service_data_type_unregister(&RLIMIT_NOFILE_HARD);
	initng_service_data_type_unregister(&RLIMIT_NPROC_SOFT);
	initng_service_data_type_unregister(&RLIMIT_NPROC_HARD);
	initng_service_data_type_unregister(&RLIMIT_RSS_SOFT);
	initng_service_data_type_unregister(&RLIMIT_RSS_HARD);
	initng_service_data_type_unregister(&RLIMIT_SIGPENDING_SOFT);
	initng_service_data_type_unregister(&RLIMIT_SIGPENDING_HARD);
	initng_service_data_type_unregister(&RLIMIT_STACK_SOFT);
	initng_service_data_type_unregister(&RLIMIT_STACK_HARD);
}
