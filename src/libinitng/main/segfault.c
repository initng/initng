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

#include <time.h>		/* time() */
#include <fcntl.h>		/* fcntl() */
#include <sys/un.h>		/* memmove() strcmp() */
#include <sys/wait.h>		/* waitpid() sa */
#include <linux/kd.h>		/* KDSIGACCEPT */
#include <sys/ioctl.h>		/* ioctl() */
#include <stdlib.h>		/* free() exit() */
#include <sys/reboot.h>		/* reboot() RB_DISABLE_CAD */
#include <sys/mount.h>
#include <termios.h>
#include <stdio.h>
#include <sys/klog.h>
#include <errno.h>
#ifdef HAVE_COREDUMPER
#include <google/coredumper.h>
#endif

/*
 * This is called on segfault,
 * DON'T call any other function in initng from here, not even P_, F_, W_!
 * Also, do NOT call any function that allocates memory (using malloc),
 * since the SIGSEGV might be due to a corrupted malloc arena
 */
void initng_main_segfault(void)
{
#ifdef DEBUG
	int i = 0;
#endif

	/* open a direct socket to /dev/console */
	int emergency_output = -1;

	/* if this is not init */
	if (g.i_am != I_AM_INIT) {
		/* just quit */
		_exit(99);
	}
#ifdef DEBUG
#define MESSAGE "Initng segfaulted, will wait in 20 seconds for you to start a gdb, before execve(/sbin/initng-segfault);\n"

#ifdef HAVE_COREDUMPER
	/* Google Coredumper - works very nicely, as long as the kernel
	   allows ptracing of init. Am keeping this around in case we use
	   threads or want to do network coredumping or the like */
	{
		char buf[50];

		sprintf(buf, "/dev/shm/initng-core-%i-%i", getpid(),
			(int)time(NULL));
		if (WriteCoreDump(buf) == 0)
			printf("Dumped core to %s\n", buf);
	}
#else
#if 0				/* deadlocks sometimes due to libc locks; need to use direct syscalls */
	/* Fork a new process and dump core. Works reasonably well as we're
	 * not using threads for anything. If we were, we'd need Google
	 * Coredumper */
	{
		int newpid = fork();

		if (newpid == 0) {
			struct sigaction act;
			sigset_t set;
			struct rlimit rlim;

			/* disable SIGSEGV signal handler */
			act.sa_handler = SIG_DFL;
			sigemptyset(&act.sa_mask);
			act.sa_flags = 0;
			sigaction(SIGSEGV, &act, NULL);

			/* unmask SIGSEGV */
			sigemptyset(&set);
			sigaddset(&set, SIGSEGV);
			sigprocmask(SIG_UNBLOCK, &set, NULL);

			/* prepare to dump core */
			chdir("/dev/shm");
			rlim.rlim_cur = RLIM_INFINITY;
			rlim.rlim_max = RLIM_INFINITY;
			setrlimit(RLIMIT_CORE, &rlim);

			/* try to dump core */
			kill(getpid(), SIGSEGV);
			_exit(0);
		} else if (newpid > 0) {
			int ret = waitpid(newpid, NULL, 0);

#ifdef 	WCOREDUMP
			if (WIFSIGNALED(ret) && WCOREDUMP(ret))
				printf("Core dumped!\n");
			else
				printf("Failed to dump core\n");
#else
			if (!WIFSIGNALED(ret))
				printf("Failed to dump core\n");
#endif
		} else {
			printf("Failed to fork and dump core\n");
		}
	}
#endif				/* false */
#endif				/* !HAVE_COREDUMPER */

	/* Try to launch a getty, that we may if lucky login to tty9 and run gdb there */
#ifdef TRY_LAUCNH_GETTY_ON_SEGFAULT
	if (fork() == 0) {
		const char *getty_argv[] =
		    { "/sbin/getty", "38400", "tty9", NULL };
		const char *getty_env[] = { NULL };

		/* execve getty in the fork */
		execve((char *)getty_argv[0], (char **)getty_argv,
		       (char **)getty_env);
		_exit(1);
	}
#endif

	/* if we compiled debug mode, we get the user 20 seconds to fire up
	 * gdb, and attach pid 1 */
	while (i < 5) {
		emergency_output = open("/dev/console", O_WRONLY);
		if (emergency_output > 0) {
			write(emergency_output, MESSAGE, strlen(MESSAGE));
			close(emergency_output);
		}
		sleep(4);	/*  5 times 4 is 20 seconds */
		i++;
	}
#endif

#define LMESSAGE "Launching /sbin/initng-segfault"
	emergency_output = open("/dev/console", O_WRONLY);
	if (emergency_output > 0) {
		write(emergency_output, LMESSAGE,
		      sizeof(char) * strlen(LMESSAGE));
		close(emergency_output);
	}

	/* launch initng-segfault */
	{
		const char *segfault_argv[] = { "/sbin/initng-segfault", NULL };
		const char *segfault_argv_initng[] =
		    { "/sbin/initng", "--hot_reload", NULL };
		const char *segfault_env[] = { NULL };

		/* first try /sbin/initng-segfault */
		if (execve((char *)segfault_argv[0], (char **)segfault_argv,
			   (char **)segfault_env) == -1) {
			/* then try /sbin/initng --hot_reload */
			if (execve((char *)segfault_argv_initng[0],
				   (char **)segfault_argv,
				   (char **)segfault_env) == -1) {
				/* else tell the user, it dont succeded to
				 * start a replacement */
				fprintf(stderr, "/sbin/initng-segfault did "
					"not exist, will die!\n");
			}
		}

	}

	/* Loop forever ever, because init can't die */
	while (1) {
		sleep(30);
		fprintf(stderr, "INITNG_SEGFAULT\n");
	}
}
