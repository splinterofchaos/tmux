/* $Id$ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
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

#include <getopt.h>

#include "tmux.h"

/*
 * Bind a key to a command, this recurses through cmd_*.
 */

int	cmd_bind_key_parse(struct cmd *, void **, int, char **, char **);
void	cmd_bind_key_exec(void *, struct cmd_ctx *);
void	cmd_bind_key_send(void *, struct buffer *);
void	cmd_bind_key_recv(void **, struct buffer *);
void	cmd_bind_key_free(void *);

struct cmd_bind_key_data {
	int		 key;
	struct cmd	*cmd;
};

const struct cmd_entry cmd_bind_key_entry = {
	"bind-key", "bind",
	"key command [arguments]",
	0,
	cmd_bind_key_parse,
	cmd_bind_key_exec,
	cmd_bind_key_send,
	cmd_bind_key_recv,
	cmd_bind_key_free
};

int
cmd_bind_key_parse(
    struct cmd *self, void **ptr, int argc, char **argv, char **cause)
{
	struct cmd_bind_key_data	*data;
	int				 opt;

	*ptr = data = xmalloc(sizeof *data);
	data->cmd = NULL;

	while ((opt = getopt(argc, argv, "")) != EOF) {
		switch (opt) {
		default:
			goto usage;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc < 1)
		goto usage;

	if ((data->key = key_string_lookup_string(argv[0])) == KEYC_NONE) {
		xasprintf(cause, "unknown key: %s", argv[0]);
		goto error;
	}

	argc--;
	argv++;
	if ((data->cmd = cmd_parse(argc, argv, cause)) == NULL)
		goto error;

	return (0);

usage:
	usage(cause, "%s %s", self->entry->name, self->entry->usage);

error:
	cmd_bind_key_free(data);
	return (-1);
}

void
cmd_bind_key_exec(void *ptr, unused struct cmd_ctx *ctx)
{
	struct cmd_bind_key_data	*data = ptr;

	if (data == NULL)
		return;

	key_bindings_add(data->key, data->cmd);
	data->cmd = NULL;	/* avoid free */

	if (ctx->cmdclient != NULL)
		server_write_client(ctx->cmdclient, MSG_EXIT, NULL, 0);
}

void
cmd_bind_key_send(void *ptr, struct buffer *b)
{
	struct cmd_bind_key_data	*data = ptr;

	buffer_write(b, data, sizeof *data);
	cmd_send(data->cmd, b);
}

void
cmd_bind_key_recv(void **ptr, struct buffer *b)
{
	struct cmd_bind_key_data	*data;

	*ptr = data = xmalloc(sizeof *data);
	buffer_read(b, data, sizeof *data);
	data->cmd = cmd_recv(b);
}

void
cmd_bind_key_free(void *ptr)
{
	struct cmd_bind_key_data	*data = ptr;

	if (data->cmd != NULL)
		cmd_free(data->cmd);
	xfree(data);
}
