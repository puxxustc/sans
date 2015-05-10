/*
 * async_connect.h - async connect
 *
 * Copyright (C) 2014 - 2015, Xiaoxiao <i@xiaoxiao.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __MINGW32__
#  include "win.h"
#else
#  include <netdb.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#endif

#include "async_connect.h"
#include "ev.h"
#include "global.h"
#include "log.h"
#include "utils.h"

#define UNUSED(x) do {(void)(x);} while (0)

typedef struct
{
	void (*cb)(int, void *);
	void *data;
	ev_io w;
} ctx_t;

static void connect_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)(w->data);

	UNUSED(revents);
	assert(ctx != NULL);

	ev_io_stop(EV_A_ w);

	if (getsockerror(w->fd) == 0)
	{
		// 连接成功
		(ctx->cb)(w->fd, ctx->data);
		free(ctx);
	}
	else
	{
		// 连接失败
		LOG("connect failed");
		close(w->fd);
		(ctx->cb)(-1, ctx->data);
		free(ctx);
	}
}

void async_connect(const struct sockaddr *addr, socklen_t addrlen,
                   void (*cb)(int, void *), void *data)
{
	ctx_t *ctx = (ctx_t *)malloc(sizeof(ctx_t));
	if (ctx == NULL)
	{
		LOG("out of memory");
		(cb)(-1, data);
		return;
	}
	ctx->cb = cb;
	ctx->data = data;

	int sock = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
	{
		ERROR("socket");
		free(ctx);
		(cb)(-1, data);
		return;
	}
	setnonblock(sock);
	settimeout(sock);
	setkeepalive(sock);
#ifdef SO_NOSIGPIPE
	setnosigpipe(sock);
#endif
	if (connect(sock, addr, addrlen) != 0)
	{
		if (errno != EINPROGRESS)
		{
			// 连接失败
			LOG("connect failed");
			close(sock);
			free(ctx);
			(cb)(-1, data);
			return;
		}
	}
	ev_io_init(&(ctx->w), connect_cb, sock, EV_WRITE);
	ctx->w.data = (void *)ctx;
	ev_io_start(g_loop, &(ctx->w));
}
