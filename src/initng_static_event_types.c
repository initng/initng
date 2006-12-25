/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 * Copyright (C) 2006 Ismael Luceno <ismael.luceno@gmail.com>
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

#include <string.h>
#include <assert.h>

#include "initng.h"
#include "initng_list.h"
#include "initng_global.h"
#include "initng_event_types.h"
#include "initng_static_event_types.h"

s_event_type EVENT_STATE_CHANGE = { "state_change", "When an active change its status, this event will apere" };
s_event_type EVENT_SYSTEM_CHANGE = { "system_change", "Triggered when system state changes" };
s_event_type EVENT_IS_CHANGE = { "is_change", "Called when the rough state of a service changes" };
s_event_type EVENT_UP_MET = { "up_met", "Triggered when a service is trying to set the RUNNING state, is a up test" };
s_event_type EVENT_MAIN = { "main", "Triggered every main loop" };

s_event_type EVENT_INTERRUPT = { "interrupt", "When initng gets an sysreq, it will get here" };
s_event_type HALT = { "halt", "Initng got a request to halt" };
s_event_type REBOOT = { "reboot", "Initng got a request to reboot" };

void initng_register_static_event_types(void)
{
    initng_event_type_register(&EVENT_STATE_CHANGE);
    initng_event_type_register(&EVENT_SYSTEM_CHANGE);
    initng_event_type_register(&EVENT_IS_CHANGE);
    initng_event_type_register(&EVENT_UP_MET);
    initng_event_type_register(&EVENT_MAIN);

    initng_event_type_register(&EVENT_INTERRUPT);
    initng_event_type_register(&HALT);
    initng_event_type_register(&REBOOT);
}

