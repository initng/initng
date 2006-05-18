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

#include <assert.h>

#include <initng_global.h>
#include <initng_active_state.h>
#include <initng_active_db.h>
#include <initng_process_db.h>
#include <initng_service_cache.h>
#include <initng_handler.h>
#include <initng_active_db.h>
#include <initng_toolbox.h>
#include <initng_plugin_hook.h>
#include <initng_load_module.h>
#include <initng_plugin_callers.h>
#include <initng_error.h>
#include <initng_plugin.h>
#include <initng_static_states.h>
#include <initng_control_command.h>

#include <initng-paths.h>

#include <stdlib.h>
#include <string.h>
#include <selinux/selinux.h>
#include <selinux/context.h>
#include <sepol/sepol.h>

INITNG_PLUGIN_MACRO;

s_entry SELINUX_CONTEXT = { "selinux_context", STRING, NULL,
	"The selinux context to start in."
};


static int set_selinux_context(active_db_h * s, process_h * p
							   __attribute__ ((unused)))
{
	const char *selinux_context = get_string(&SELINUX_CONTEXT, s);
	char *sestr = NULL;
	context_t seref = NULL;
	int rc = 0;
	char *sedomain;
	int enforce = -1;

	if (selinux_context)
	{
		sedomain = (char *) malloc((sizeof(char) * strlen(selinux_context) +
									1));
		strcpy(sedomain, selinux_context);
	}
	else
	{
		sedomain = (char *) malloc((sizeof(char) * 9));
		strcpy(sedomain, "initrc_t");
	}
	rc = getcon(&sestr);
	if (rc < 0)
		goto fail;
	seref = context_new(sestr);
	if (!seref)
		goto fail;
	if (context_type_set(seref, sedomain))
		goto fail;
	freecon(sestr);
	sestr = context_str(seref);
	if (!sestr)
		goto fail;
	rc = setexeccon(sestr);
	if (rc < 0)
		goto fail;
	return (TRUE);

  fail:
	selinux_getenforcemode(&enforce);
	if (enforce)
	{
		F_("bash_this(): could not change selinux context!\n ERROR!\n");
		return (FALSE);
	}
	else
	{
		return (TRUE);
	}
}


/*older code no longer needed on FC5 and FCX (X>=5) */
#ifdef OLDSELINUX
/* Mount point for selinuxfs. */
#define SELINUXMNT "/selinux/"
int enforcing = -1;

static int load_policy(int *enforce)
{
	int fd = -1, ret = -1;
	size_t data_size;
	int rc = 0, orig_enforce;
	struct stat sb;
	void *map, *data;
	char policy_file[PATH_MAX];
	int policy_version = 0;
	FILE *cfg;
	char buf[4096];
	int seconfig = -2;
	char *nonconst;				//Ugly hack!

	selinux_getenforcemode(&seconfig);

	mount("none", "/proc", "proc", 0, 0);
	cfg = fopen("/proc/cmdline", "r");
	if (cfg)
	{
		char *tmp;

		if (fgets(buf, 4096, cfg) && (tmp = strstr(buf, "enforcing=")))
		{
			if (tmp == buf || isspace(*(tmp - 1)))
			{
				enforcing = atoi(tmp + 10);
			}
		}
		fclose(cfg);
	}
#define MNT_DETACH 2
	umount2("/proc", MNT_DETACH);

	if (enforcing >= 0)
		*enforce = enforcing;
	else if (seconfig == 1)
		*enforce = 1;

	if (mount("none", SELINUXMNT, "selinuxfs", 0, 0) < 0)
	{
		if (errno == ENODEV)
		{
			fprintf(stderr, "SELinux not supported by kernel: %s\n",
					strerror(errno));
			*enforce = 0;
		}
		else
		{
			fprintf(stderr, "Failed to mount %s: %s\n", SELINUXMNT,
					strerror(errno));
		}
		return ret;
	}

	nonconst = malloc(sizeof(SELINUXMNT));
	strcpy(nonconst, SELINUXMNT);
	set_selinuxmnt(nonconst);				/* set manually since we mounted it */
	free(nonconst);

	policy_version = security_policyvers();
	if (policy_version < 0)
	{
		fprintf(stderr, "Can't get policy version: %s\n", strerror(errno));
		goto UMOUNT;
	}

	orig_enforce = rc = security_getenforce();
	if (rc < 0)
	{
		fprintf(stderr, "Can't get SELinux enforcement flag: %s\n",
				strerror(errno));
		goto UMOUNT;
	}
	if (enforcing >= 0)
	{
		*enforce = enforcing;
	}
	else if (seconfig == -1)
	{
		*enforce = 0;
		rc = security_disable();
		if (rc == 0)
			umount(SELINUXMNT);
		if (rc < 0)
		{
			rc = security_setenforce(0);
			if (rc < 0)
			{
				fprintf(stderr, "Can't disable SELinux: %s\n",
						strerror(errno));
				goto UMOUNT;
			}
		}
		ret = 0;
		goto UMOUNT;
	}
	else if (seconfig >= 0)
	{
		*enforce = seconfig;
		if (orig_enforce != *enforce)
		{
			rc = security_setenforce(seconfig);
			if (rc < 0)
			{
				fprintf(stderr, "Can't set SELinux enforcement flag: %s\n",
						strerror(errno));
				goto UMOUNT;
			}
		}
	}

	snprintf(policy_file, sizeof(policy_file), "%s.%d",
			 selinux_binary_policy_path(), policy_version);
	fd = open(policy_file, O_RDONLY);
	if (fd < 0)
	{
		/* Check previous version to see if old policy is available
		 */
		snprintf(policy_file, sizeof(policy_file), "%s.%d",
				 selinux_binary_policy_path(), policy_version - 1);
		fd = open(policy_file, O_RDONLY);
		if (fd < 0)
		{
			fprintf(stderr, "Can't open '%s.%d':  %s\n",
					selinux_binary_policy_path(), policy_version,
					strerror(errno));
			goto UMOUNT;
		}
	}

	if (fstat(fd, &sb) < 0)
	{
		fprintf(stderr, "Can't stat '%s':  %s\n",
				policy_file, strerror(errno));
		goto UMOUNT;
	}

	map = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED)
	{
		fprintf(stderr, "Can't map '%s':  %s\n",
				policy_file, strerror(errno));
		goto UMOUNT;
	}


	/* Set SELinux users based on a local.users configuration file. */
	ret = sepol_genusers(map, sb.st_size, selinux_users_path(), &data,
						 &data_size);
	if (ret < 0)
	{
		fprintf(stderr,
				"Warning!  Error while reading user configuration from %s/{local.users,system.users}:  %s\n",
				selinux_users_path(), strerror(errno));
		data = map;
		data_size = sb.st_size;
	}

	/* Set booleans based on a booleans configuration file. */
	nonconst = malloc(sizeof(selinux_booleans_path()));
	strcpy(nonconst, selinux_booleans_path());
	ret = sepol_genbools(data, data_size, nonconst);
	free(nonconst);
	if (ret < 0)
	{
		if (errno == ENOENT || errno == EINVAL)
		{
			/* No booleans file or stale booleans in the file; non-fatal. */
			fprintf(stderr, "Warning!  Error while setting booleans:  %s\n",
					strerror(errno));
		}
		else
		{
			fprintf(stderr, "Error while setting booleans:  %s\n",
					strerror(errno));
			goto UMOUNT;
		}
	}
	fprintf(stderr, "Loading security policy\n");
	ret = security_load_policy(data, data_size);
	if (ret < 0)
	{
		fprintf(stderr, "security_load_policy failed\n");
	}

  UMOUNT:
	/*umount(SELINUXMNT); */
	if (fd >= 0)
	{
		close(fd);
	}
	return (ret);
}
#endif




int module_init(int api_version)
{
	D_("module_init();\n");
	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

#ifdef OLDSELINUX
	if ((fopen("/selinux/enforce", "r")) != NULL)
		goto BOOT;
	int enforce = -1;
	char *nonconst;

	if (getenv("SELINUX_INIT") == NULL)
	{
		nonconst = malloc(sizeof("SELINUX_INIT=YES"));
		strcpy(nonconst, "SELINUX_INIT=YES");
		putenv(nonconst);
		free(nonconst);
		if (load_policy(&enforce) == 0)
		{
			execv(g.Argv[0], g.Argv);
		}
		else
		{
			if (enforce > 0)
			{
				/* SELinux in enforcing mode but load_policy failed */
				/* At this point, we probably can't open /dev/console, so log() won't work */
				fprintf(stderr,
						"Enforcing mode requested but no policy loaded. Halting now.\n");
				exit(1);
			}
		}
	}

  BOOT:
#endif
#ifndef OLDSELINUX
	int enforce = 0;
	char *envstr;

	if (getenv("SELINUX_INIT") == NULL)
	{
		envstr = malloc(sizeof("SELINUX_INIT=YES"));
		strcpy(envstr, "SELINUX_INIT=YES");
		putenv(envstr);
		if (selinux_init_load_policy(&enforce) == 0)
		{
			execv(g.Argv[0], g.Argv);
		}
		else
		{
			if (enforce > 0)
			{
				/* SELinux in enforcing mode but load_policy failed */
				/* At this point, we probably can't open /dev/console, so log() won't work */
				fprintf(stderr,
						"Unable to load SELinux Policy. Machine is in enforcing mode. Halting now.");
				exit(1);
			}
		}
	}
#endif


	/* add hokks and malloc data here */

	initng_service_data_type_register(&SELINUX_CONTEXT);

	initng_plugin_hook_register(&g.A_FORK, 20, &set_selinux_context);

	return (TRUE);
}


void module_unload(void)
{
	D_("module_unload();\n");

	/* remove hooks and free data here */
	initng_service_data_type_unregister(&SELINUX_CONTEXT);
	initng_plugin_hook_unregister(&g.A_FORK, &set_selinux_context);
}
