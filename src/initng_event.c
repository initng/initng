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

#include <assert.h>
#include "initng.h"
#include "initng_plugin.h"
#include "initng_event_types.h"
#include "initng_event.h"

int initng_event_send(s_event *event)
{
	s_call *current;
	int ret;

	assert(event);
	assert(event->event_type);

	D_("%s event triggered\n", event->event_type->name);

	while_list(current, &event->event_type->hooks)
	{
		ret = current->c.event(event);

		if (ret == FAIL)
			return FAIL;

		if (ret == event->break_on)
		{
			D_("%s event aborted (returned %d)\n", event->event_type->name, ret);
			return (FALSE);
		}
	}

	return (TRUE);
}
