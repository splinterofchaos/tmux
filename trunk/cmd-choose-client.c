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
 * Enter choice mode to choose a client.
 */

int	cmd_choose_client_exec(struct cmd *, struct cmd_ctx *);

void	cmd_choose_client_callback(void *);
void	cmd_choose_client_free(void *);

const struct cmd_entry cmd_choose_client_entry = {
	"choose-client", NULL,
	"F:t:", 0, 1,
	CMD_TARGET_WINDOW_USAGE " [-F format] [template]",
	0,
	NULL,
	NULL,
	cmd_choose_client_exec
};

struct cmd_choose_client_data {
	struct client	*client;
	char   		*template;
};

int
cmd_choose_client_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct args			*args = self->args;
	struct window_choose_data	*cdata;
	struct format_tree		*ft;
	struct winlink			*wl;
	struct client			*c;
	const char			*template;
	u_int			 	 i, idx, cur;

	if (ctx->curclient == NULL) {
		ctx->error(ctx, "must be run interactively");
		return (-1);
	}

	if ((wl = cmd_find_window(ctx, args_get(args, 't'), NULL)) == NULL)
		return (-1);

	if (window_pane_set_mode(wl->window->active, &window_choose_mode) != 0)
		return (0);

	if ((template = args_get(args, 'F')) == NULL)
		template = DEFAULT_CLIENT_TEMPLATE;

	cur = idx = 0;
	for (i = 0; i < ARRAY_LENGTH(&clients); i++) {
		c = ARRAY_ITEM(&clients, i);
		if (c == NULL || c->session == NULL)
			continue;
		if (c == ctx->curclient)
			cur = idx;
		idx++;

		ft = format_create();

		cdata = xmalloc(sizeof *cdata);
		if (args->argc != 0)
			cdata->action = xstrdup(args->argv[0]);
		else
			cdata->action = xstrdup("detach-client -t '%%'");
		cdata->client = ctx->curclient;
		cdata->idx = i;
		cdata->ft_template = xstrdup(template);
		cdata->ft = ft;

		format_add(ft, "line", "%u", i);
		format_session(ft, c->session);
		format_client(ft, c);

		window_choose_add(wl->window->active, cdata);

		format_free(ft);
	}

	cdata->client->references++;

	window_choose_ready(wl->window->active,
	    cur, cmd_choose_client_callback, cmd_choose_client_free);

	return (0);
}

void
cmd_choose_client_callback(void *data)
{
	struct window_choose_data	*cdata = data;
	struct client  			*c;
	u_int				 idx;

	if (cdata == NULL)
		return;
	if (cdata->client->flags & CLIENT_DEAD)
		return;

	idx = cdata->idx;

	if (idx > ARRAY_LENGTH(&clients) - 1)
		return;
	c = ARRAY_ITEM(&clients, idx);
	if (c == NULL || c->session == NULL)
		return;

	xasprintf(&cdata->raw_format, "%s", c->tty.path);
	window_choose_ctx(cdata);
}

void
cmd_choose_client_free(void *data)
{
	struct window_choose_data	*cdata = data;

	cdata->client->references--;

	/* TA:  FIXME - move this to window_choose_free() or somesuch. */
	xfree((char *)cdata->ft_template);
	xfree(cdata->action);
	xfree(cdata->raw_format);
	xfree(cdata);
}
