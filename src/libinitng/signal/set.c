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

#include <stdlib.h>				/* free() exit() */
#include <stdio.h>				/* printf() */
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <initng.h>

#include "local.h"


volatile int signals_got[SIGNAL_STACK];

struct sigaction sa;

static void set_signal(int sig)
{
	int i;

	/*printf("signal: %i\n", sig); */
	for (i = 0; i < SIGNAL_STACK; i++) {
		/* check if this signaltype is already
		 * on the list of signals */
		if (signals_got[i] == sig)
			return;

		/* else add this on a free spot */
		if (signals_got[i] == -1) {
			/*printf("signals_got[%i]=%i\n", i, sig); */
			signals_got[i] = sig;
			return;
		}
	}
	F_("SIGNAL STACK FULL!\n");
}

/*
 * This is called in main() initialization, and will enable signals when it
 * happens.
 */
void initng_signal_enable(void)
{
	int i;

	D_("enable_signals();\n");

	/* clear interrupt cache */
	/* TODO FIGUREITOUT - Catch signals */
	/*  signal(SIGPWR,sighandler); don't know what to do about it */
	/*  signal(SIGHUP,sighandler); signal for plug-ins to do something */
	/*  (recreate control file for example) */
	/* SA_NOCLDSTOP -->  Get notification when child stops living */
	/* SA_RESTART --> make certain system calls restartable across signals */
	/*   Normally if a program is in a system call and a signal is */
	/*   delivered, the system call exits with and error sets errno */
	/*   to EINTR. This is quite annoying in practice, as *every* */
	/*   system call has to done in a loop, retrying if the error */
	/*   EINTR. By setting SA_RESTART, system calls will "restart", */
	/*   continuing normally when your signal handler returns. */

	/* create a signal set */
	sigemptyset(&sa.sa_mask);

	/* i don't get this one */
	sa.sa_sigaction = 0;

	/* clear signal */
	for (i = 0; i < SIGNAL_STACK; i++)
		signals_got[i] = -1;

	/* SA_NOCLDSTOP = Don't give initng signal if we kill the app with SIGSTOP */
	/* SA_RESTART = call signal over again next time */
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

	/* segfault */
	if (g.i_am == I_AM_INIT) {
		sa.sa_handler = sigsegv;
		sigaction(SIGSEGV, &sa, 0);
		sigaction(SIGABRT, &sa, 0);
	}

	sa.sa_handler = set_signal;
	sigaction(SIGCHLD, &sa, 0);		/* Dead children */
	sigaction(SIGINT, &sa, 0);		/* ctrl-alt-del */
	sigaction(SIGWINCH, &sa, 0);		/* keyboard request */
	sigaction(SIGALRM, &sa, 0);		/* alarm, something has to */
						/* be checked */
	sigaction(SIGHUP, &sa, 0);		/* sighup, plugin actions */
	sigaction(SIGPIPE, &sa, 0);		/* sigpipe, plugin actions */
}

/* 
 * Disable signals, called when initng are exiting.
 * sometimes, we don't want to be disturbed with signals.
 */
void initng_signal_disable(void)
{
	D_("disable_signals();\n");

	/* create a signal set */
	sigemptyset(&sa.sa_mask);

	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

	/* i don't get this one */
	sa.sa_sigaction = 0;
	sa.sa_handler = SIG_DFL;
	sigaction(SIGSEGV, &sa, 0);
	sigaction(SIGABRT, &sa, 0);
	sigaction(SIGCHLD, &sa, 0);		/* Dead children */
	sigaction(SIGINT, &sa, 0);		/* ctrl-alt-del */
	sigaction(SIGWINCH, &sa, 0);		/* keyboard request */
	sigaction(SIGALRM, &sa, 0);		/* alarm, something has to */
						/* be checked */
	sigaction(SIGHUP, &sa, 0);		/* sighup, plugin actions */
}
