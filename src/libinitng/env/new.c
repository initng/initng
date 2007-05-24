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

#include <time.h>							/* time() */
#include <fcntl.h>							/* fcntl() */
#include <sys/un.h>							/* memmove() strcmp() */
#include <sys/wait.h>						/* waitpid() sa */
#include <linux/kd.h>						/* KDSIGACCEPT */
#include <sys/ioctl.h>						/* ioctl() */
#include <stdio.h>							/* printf() */
#include <stdlib.h>							/* free() exit() */
#include <sys/reboot.h>						/* reboot() RB_DISABLE_CAD */
#include <assert.h>
#include <ctype.h>							/* isgraph */

#include <initng-paths.h>

const char *initng_environ[] = {
	"INITNG=" INITNG_VERSION,
	"INITNG_PLUGIN_DIR=" INITNG_PLUGIN_DIR,
	"INITNG_ROOT=" INITNG_ROOT,
	"PATH=/bin:/usr/bin:/sbin:/usr/sbin:/usr/local/bin:/opt/bin",
	"HOME=/root",
	"USER=root",
	"TERM=linux",
#ifdef FORCE_POSIX_IFILES
	"POSIXLY_CORRECT=1",
#endif
	NULL
};

/* creates a custom set of environment variables, for passing to exec
 * Warning: this leaks memory, so only use if you are going to exec()!
 * Don't even *think* about free()ing the strings, unless you want
 * invalid pointers in the service_db! (easy to fix, see comment below)
 */
char **new_environ(active_db_h * s)
{
	int allocate;
	int nr = 0;
	int i;
	char **env;
	char *var;

	/* Better safe than sorry... */
	if (s && s->name == NULL)
		s->name = i_strdup("unknown");

	/*
	 * FIRST, try to figure out how big array we want to create.
	 */

	/*  At least 11 allocations below, and place for plugin added to (about 100) */
	allocate = 114;

	/* count existing env's */

	/* count ENVs in service ENV variable */
	if (s)
		allocate += count_type(&ENV, s);


	/* finally allocate */
	env = (char **) i_calloc(allocate, sizeof(char *));

	/* add all static defined above in initng_environ */
	for (nr = 0; initng_environ[nr]; nr++)
	{
		env[nr] = i_strdup(initng_environ[nr]);
	}

	/* Set INITNG_PID, so we can send signals to initng */
	env[nr] = malloc(32);
	snprintf(env[nr], 32, "INITNG_PID=%d", getpid());
	env[nr][31] = '\0';
	nr++;

	if (s && (nr + 4) < allocate)
	{
		env[nr] = (char *) i_calloc(1, sizeof(char) * (9 + strlen(s->name)));
		strcpy(env[nr], "SERVICE=");
		strcat(env[nr], s->name);
		nr++;

		env[nr] = (char *) i_calloc(1, sizeof(char) * (6 + strlen(s->name)));
		strcpy(env[nr], "NAME=");
		strcat(env[nr], st_strip_path(s->name));
		nr++;

		env[nr] = (char *) i_calloc(1, sizeof(char) * (7 + strlen(s->name)));
		strcpy(env[nr], "CLASS=");
		strcat(env[nr], s->name);
		st_strip_end(&env[nr]);
		nr++;

		if (g.dev_console)
		{
			env[nr] = (char *) i_calloc(1, sizeof(char) *
			                   (9 + strlen(g.dev_console)));
			strcpy(env[nr], "CONSOLE=");
			strcat(env[nr], g.dev_console);
		}
		else
		{
			env[nr] = (char *) i_calloc(1, sizeof(char) *
			                   (9 + strlen(INITNG_CONSOLE)));
			strcpy(env[nr], "CONSOLE=");
			strcat(env[nr], INITNG_CONSOLE);
		}
		nr++;

		if (g.runlevel && (nr + 1) < allocate)
		{
			env[nr] = (char *) i_calloc(1, sizeof(char) *
			                   (10 + strlen(g.runlevel)));
			strcpy(env[nr], "RUNLEVEL=");
			strcat(env[nr], g.runlevel);
			nr++;
		}

		if (g.old_runlevel && (nr + 1) < allocate)
		{
			env[nr] = (char *) i_calloc(1, sizeof(char) *
			                   (14 + strlen(g.old_runlevel)));
			strcpy(env[nr], "PREVLEVEL=");
			strcat(env[nr], g.old_runlevel);
			nr++;
		}

		env[nr] = NULL;
		/* insert all env strings from config */
		{
			var = NULL;
			s_data *tmp = NULL;

			while ((nr + 1) < allocate)
			{
				if (!(tmp = get_next(&ENV, s, tmp)))
					break;

				/* then malloc */
				var = i_calloc((strlen(tmp->vn) + strlen(tmp->t.s) + 3),
							   sizeof(char));
				if (!var)
					continue;

				/* copy the data */
				strcpy(var, tmp->vn);
				strcat(var, "=");
				strcat(var, tmp->t.s);

				/* check for duplicates */
				for (i = 0; i < nr; i++)
				{
					if (is_same_env_var(env[i], var))
					{
						/* we may want to override environemntal variables
						   set above, particularly PATH and HOME */
						free(env[i]);
						env[i] = var;
						break;
					}
				}

				if (i == nr)
					env[nr++] = var;
			}
		}
	}

	/* null last */
	if (env[nr] != NULL)
		env[nr++] = NULL;

#ifdef DEBUG
	for (nr = 0; env[nr]; nr++)
	{
		D_("environ[%i] = \"%s\"\n", nr, env[nr]);
	}
#endif

	/* return new environ */
	return (env);
}
