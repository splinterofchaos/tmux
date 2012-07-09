/* $Id$ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
 * Copyright (c) 2010 Romain Francoise <rfrancoise@debian.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <string.h>
#include <signal.h>

#include "tmux.h"

struct event	ev_sighup;
struct event	ev_sigchld;
struct event	ev_sigcont;
struct event	ev_sigterm;
struct event	ev_sigusr1;
struct event	ev_sigwinch;

void		clear_signal(struct sigaction*, int signum);
void		set_add_signal(struct event *, void(*)(int, short, void *), int);

/* Clear signal. */
void 
clear_signal(struct sigaction* sigact, int signum)
{
	if (sigaction(signum, &sigact, NULL) != 0)
		fatal("sigaction failed");
}

/* Set and add signal. */
void 
set_add_signal(struct event *ev, void(*handler)(int, short, unused void *), 
                    int signum)
{
	signal_set(ev, signum, handler, NULL);
	signal_add(ev, NULL);
}


void
set_signals(void(*handler)(int, short, unused void *))
{
	struct sigaction	sigact;

	memset(&sigact, 0, sizeof sigact);
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	sigact.sa_handler = SIG_IGN;
	clear_signal(&sigact, SIGINT);
	clear_signal(&sigact, SIGPIPE);
	clear_signal(&sigact, SIGUSR2);
	clear_signal(&sigact, SIGTSTP);


	set_add_signal(&ev_sighup, handler, SIGHUP);
	set_add_signal(&ev_sigchld, handler, SIGCHLD);
	set_add_signal(&ev_sigcont, handler, SIGCONT);
	set_add_signal(&ev_sigterm, handler, SIGTERM);
	set_add_signal(&ev_sigusr1, handler, SIGUSR1);
	set_add_signal(&ev_sigwinch, handler, SIGWINCH);
}

void
clear_signals(int after_fork)
{
	struct sigaction	sigact;

	memset(&sigact, 0, sizeof sigact);
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	sigact.sa_handler = SIG_DFL;
	if (sigaction(SIGINT, &sigact, NULL) != 0)
		fatal("sigaction failed");
	if (sigaction(SIGPIPE, &sigact, NULL) != 0)
		fatal("sigaction failed");
	if (sigaction(SIGUSR2, &sigact, NULL) != 0)
		fatal("sigaction failed");
	if (sigaction(SIGTSTP, &sigact, NULL) != 0)
		fatal("sigaction failed");

	if (after_fork) {
		if (sigaction(SIGHUP, &sigact, NULL) != 0)
			fatal("sigaction failed");
		if (sigaction(SIGCHLD, &sigact, NULL) != 0)
			fatal("sigaction failed");
		if (sigaction(SIGCONT, &sigact, NULL) != 0)
			fatal("sigaction failed");
		if (sigaction(SIGTERM, &sigact, NULL) != 0)
			fatal("sigaction failed");
		if (sigaction(SIGUSR1, &sigact, NULL) != 0)
			fatal("sigaction failed");
		if (sigaction(SIGWINCH, &sigact, NULL) != 0)
			fatal("sigaction failed");
	} else {
		event_del(&ev_sighup);
		event_del(&ev_sigchld);
		event_del(&ev_sigcont);
		event_del(&ev_sigterm);
		event_del(&ev_sigusr1);
		event_del(&ev_sigwinch);
	}
}
