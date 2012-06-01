/* $Id$ */

/*
 * Copyright (c) 2009 Nicholas Marriott <nicm@users.sourceforge.net>
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

#include <sys/types.h>

#include <ctype.h>

#include "tmux.h"

/*
 * Enter choice mode to choose a window.
 */

int	cmd_choose_window_exec(struct cmd *, struct cmd_ctx *);

void	cmd_choose_window_callback(void *);
void	cmd_choose_window_free(void *);

const struct cmd_entry cmd_choose_window_entry = {
	"choose-window", NULL,
	"F:t:", 0, 1,
	CMD_TARGET_WINDOW_USAGE " [-F format] [template]",
	0,
	NULL,
	NULL,
	cmd_choose_window_exec
};

int
cmd_choose_window_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct args			*args = self->args;
	struct window_choose_data	*cdata;
	struct session			*s;
	struct winlink			*wl, *wm;
	struct format_tree		*ft;
	const char			*template;
	u_int			 	 idx, cur;

	if (ctx->curclient == NULL) {
		ctx->error(ctx, "must be run interactively");
		return (-1);
	}
	s = ctx->curclient->session;

	if ((wl = cmd_find_window(ctx, args_get(args, 't'), NULL)) == NULL)
		return (-1);

	if (window_pane_set_mode(wl->window->active, &window_choose_mode) != 0)
		return (0);

	if ((template = args_get(args, 'F')) == NULL)
		template = DEFAULT_WINDOW_TEMPLATE;

	cur = idx = 0;
	RB_FOREACH(wm, winlinks, &s->windows) {
		if (wm == s->curw)
			cur = idx;
		idx++;

		ft = format_create();
		cdata = xmalloc(sizeof *cdata);
		if (args->argc != 0)
			cdata->action = xstrdup(args->argv[0]);
		else
			cdata->action = xstrdup("select-window -t '%%'");
		cdata->session = s;
		cdata->client = ctx->curclient;
		cdata->idx = idx;
		cdata->ft = ft;

		format_add(ft, "line", "%u", idx);
		format_session(ft, s);
		format_winlink(ft, s, wm);

		window_choose_add(wl->window->active, cdata);

		format_free(ft);
	}

	cdata->session->references++;
	cdata->client->references++;

	window_choose_ready(wl->window->active,
	    cur, cmd_choose_window_callback, cmd_choose_window_free);

	return (0);
}

void
cmd_choose_window_callback(void *data)
{
	struct window_choose_data	*cdata = data;
	struct session			*s = cdata->session;
	u_int				 idx;

	if (cdata == NULL)
		return;
	if (!session_alive(s))
		return;
	if (cdata->client->flags & CLIENT_DEAD)
		return;

	xasprintf(&cdata->raw_format, "%s:%d", s->name, idx);
	window_choose_ctx(cdata);
}

void
cmd_choose_window_free(void *data)
{
	struct window_choose_data	*cdata = data;

	cdata->session->references--;
	cdata->client->references--;

	/* TA:  FIXME - move this to window_choose_free() or somesuch. */
	xfree((char *)cdata->ft_template);
	xfree(cdata->action);
	xfree(cdata->raw_format);
	xfree(cdata);
}
