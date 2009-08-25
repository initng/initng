/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2006 Ismael Luceno <ismael.luceno@gmail.com>
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

#include <string.h>
#include <assert.h>

#include <initng.h>

s_event_type EVENT_STATE_CHANGE = {
	.name = "state_change",
	.description = "When an active change it's status, this event will "
	    "be triggered"
};

s_event_type EVENT_SYSTEM_CHANGE = {
	.name = "system_change",
	.description = "Triggered when system state changes"
};

s_event_type EVENT_IS_CHANGE = {
	.name = "is_change",
	.description = "Triggered when the rough state of a service changes"
};

s_event_type EVENT_UP_MET = {
	.name = "up_met",
	.description = "Triggered when a service is trying to set the "
	    "RUNNING state, is a up test"
};

s_event_type EVENT_MAIN = {
	.name = "main",
	.description = "Triggered every main loop"
};

s_event_type EVENT_LAUNCH = {
	.name = "launch",
	.description = "Triggered when a process are getting launched"
};

s_event_type EVENT_AFTER_FORK = {
	.name = "after_fork",
	.description = "Triggered after a process forks to start"
};

s_event_type EVENT_START_DEP_MET = {
	.name = "start_dep_met",
	.description = "Triggered when a service is about to start"
};

s_event_type EVENT_STOP_DEP_MET = {
	.name = "stop_dep_met",
	.description = "Triggered when a service is about to stop"
};

s_event_type EVENT_PIPE_WATCHER = {
	.name = "pipe_watcher",
	.description = "Watch pipes for communication"
};

s_event_type EVENT_NEW_ACTIVE = {
	.name = "new_active",
	.description = "Triggered when initng tries to resolve a nonexistent "
	    "service to start"
};

s_event_type EVENT_DEP_ON = {
	.name = "dep_on",
	.description = "Triggered when a function tries to find out a "
	    "service dependency"
};

s_event_type EVENT_RELOAD_ACTIVE_DB = {
	.name = "reload_active_db",
	.description = "Asks for a plugin willing to reload the active_db "
	    "from a dump"
};

s_event_type EVENT_DUMP_ACTIVE_DB = {
	.name = "dump_active_db",
	.description = "Asks for a plugin willing to dump the active_db"
};

s_event_type EVENT_ERROR_MESSAGE = {
	.name = "error_message",
	.description = "Triggered when an error message is sent, so all "
	    "output plug-ins can show it"
};

s_event_type EVENT_COMPENSATE_TIME = {
	.name = "compensate_time",
	.description = "Triggered when initng detects a system time change"
};

s_event_type EVENT_HANDLE_KILLED = {
	.name = "handle_killed",
	.description = "Triggered when a process dies"
};

s_event_type EVENT_SIGNAL = {
	.name = "signal",
	.description = "Triggered when initng rescives a signal, like SIGHUP"
};

s_event_type EVENT_BUFFER_WATCHER = {
	.name = "buffer_watcher",
	.description = "Triggered when a service have outputed, and initng "
	    "have filled its output buffer"
};
s_event_type EVENT_IO_WATCHER = {
	.name = "io_watcher",
	.description = "Triggered when initng open file descriptors receive "
	    "data"
};

s_event_type EVENT_INTERRUPT = {
	.name = "interrupt",
	.description = "When initng gets an sysreq, it will get here"
};

s_event_type HALT = {
	.name = "halt",
	.description = "Initng got a request to halt"
};

s_event_type REBOOT = {
	.name = "reboot",
	.description = "Initng got a request to reboot"
};

void initng_register_static_event_types(void)
{
	initng_event_type_register(&EVENT_STATE_CHANGE);
	initng_event_type_register(&EVENT_SYSTEM_CHANGE);
	initng_event_type_register(&EVENT_IS_CHANGE);
	initng_event_type_register(&EVENT_UP_MET);
	initng_event_type_register(&EVENT_MAIN);
	initng_event_type_register(&EVENT_LAUNCH);
	initng_event_type_register(&EVENT_AFTER_FORK);
	initng_event_type_register(&EVENT_START_DEP_MET);
	initng_event_type_register(&EVENT_STOP_DEP_MET);
	initng_event_type_register(&EVENT_PIPE_WATCHER);
	initng_event_type_register(&EVENT_NEW_ACTIVE);
	initng_event_type_register(&EVENT_DEP_ON);
	initng_event_type_register(&EVENT_RELOAD_ACTIVE_DB);
	initng_event_type_register(&EVENT_DUMP_ACTIVE_DB);
	initng_event_type_register(&EVENT_ERROR_MESSAGE);
	initng_event_type_register(&EVENT_COMPENSATE_TIME);
	initng_event_type_register(&EVENT_HANDLE_KILLED);
	initng_event_type_register(&EVENT_SIGNAL);
	initng_event_type_register(&EVENT_BUFFER_WATCHER);
	initng_event_type_register(&EVENT_IO_WATCHER);
	initng_event_type_register(&EVENT_INTERRUPT);
	initng_event_type_register(&HALT);
	initng_event_type_register(&REBOOT);
}
