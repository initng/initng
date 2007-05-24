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

/* NOTE: Older code, no longer needed on FC5 and later */

#include <unistd.h>
#include <time.h>				/* time() */
#include <fcntl.h>				/* fcntl() */
#include <linux/kd.h>				/* KDSIGACCEPT */
#include <stdlib.h>				/* free() exit() */
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <fnmatch.h>
#include <errno.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/klog.h>
#include <sys/reboot.h>				/* reboot() RB_DISABLE_CAD */
#include <sys/ioctl.h>				/* ioctl() */
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/un.h>				/* memmove() strcmp() */
#include <sys/wait.h>				/* waitpid() sa */
#include <sys/mount.h>

#include "selinux.h"

/* Mount point for selinuxfs. */
#define SELINUXMNT "/selinux/"
int enforcing = -1;

int load_policy(int *enforce)
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
		close(fd);

	return (ret);
}
