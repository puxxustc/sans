/*
 * socks5.c - SOCKS5 Protocol
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __MINGW32__
#  include "win.h"
#else
#  include <netdb.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#endif

#include "ev.h"
#include "log.h"
#include "global.h"
#include "socks5.h"
#include "utils.h"

#define UNUSED(x) do {(void)(x);} while (0)

typedef enum
{
	CLOSED = 0,
	HELLO_SENT,
	HELLO_RCVD,
	REQ_SENT,
	ESTAB
} state_t;

typedef struct
{
	state_t state;
	int sock;
	socklen_t addrlen;
	void (*cb)(int, void *);
	const struct sockaddr *addr;
	void *data;
	ev_io w_read;
	ev_io w_write;
} ctx_t;

static void connect_cb(EV_P_ ev_io *w, int revents);
static void socks5_send_cb(EV_P_ ev_io *w, int revents);
static void socks5_recv_cb(EV_P_ ev_io *w, int revents);

static struct
{
	socklen_t addrlen;
	struct sockaddr_storage addr;
} socks5;

int socks5_init(const char *host, const char *port)
{
	// 解析 socks5 地址
	struct addrinfo hints;
	struct addrinfo *res;
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	if (getaddrinfo(host, port, &hints, &res) != 0)
	{
		ERROR("getaddrinfo");
		return -1;
	}
	memcpy(&socks5.addr, res->ai_addr, res->ai_addrlen);
	socks5.addrlen = res->ai_addrlen;
	freeaddrinfo(res);

	return 0;
}

void socks5_connect(const struct sockaddr *addr, socklen_t addrlen,
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
	ctx->addr = addr;
	ctx->addrlen = addrlen;
	ctx->data = data;

	// 连接 SOCKS5 server
	ctx->sock = socket(socks5.addr.ss_family, SOCK_STREAM, IPPROTO_TCP);
	if (ctx->sock < 0)
	{
		ERROR("socket");
		(ctx->cb)(-1, ctx->data);
		free(ctx);
		return;
	}
	setnonblock(ctx->sock);
	settimeout(ctx->sock);
	setkeepalive(ctx->sock);
	if (connect(ctx->sock, (struct sockaddr *)&socks5.addr, socks5.addrlen) != 0)
	{
		if (errno != EINPROGRESS)
		{
			// 连接失败
			LOG("connect to SOCKS5 server failed");
			close(ctx->sock);
			(ctx->cb)(-1, ctx->data);
			free(ctx);
			return;
		}
	}
	ev_io_init(&ctx->w_write, connect_cb, ctx->sock, EV_WRITE);
	ctx->w_write.data = (void *)ctx;
	ev_io_start(g_loop, &ctx->w_write);
}

static void connect_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)(w->data);

	UNUSED(revents);
	assert(ctx != NULL);

	ev_io_stop(EV_A_ w);

	if (getsockerror(w->fd) == 0)
	{
		// 连接成功
		ctx->state = CLOSED;
		ev_io_init(&ctx->w_read, socks5_recv_cb, ctx->sock, EV_READ);
		ev_io_init(&ctx->w_write, socks5_send_cb, ctx->sock, EV_WRITE);
		ctx->w_read.data = (void *)ctx;
		ctx->w_write.data = (void *)ctx;
		ev_io_start(EV_A_ &ctx->w_write);
	}
	else
	{
		LOG("connect to SOCKS5 server failed");
		close(ctx->sock);
		(ctx->cb)(-1, ctx->data);
		free(ctx);
		return;
	}
}

static void socks5_send_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)(w->data);

	UNUSED(revents);
	assert(ctx != NULL);

	ev_io_stop(EV_A_ w);

	uint8_t buf[263];
	ssize_t len = 0;

	switch (ctx->state)
	{
	case CLOSED:
	{
		buf[0] = 0x05;
		buf[1] = 0x01;
		buf[2] = 0x00;
		len = 3;
		ctx->state = HELLO_SENT;
		break;
	}
	case HELLO_RCVD:
	{
		buf[0] = 0x05;
		buf[1] = 0x01;
		buf[2] = 0x00;
		if (ctx->addr->sa_family == AF_INET)
		{
			buf[3] = 0x01;
			memcpy(buf + 4, &(((struct sockaddr_in *)ctx->addr)->sin_addr), 4);
			memcpy(buf + 8, &(((struct sockaddr_in *)ctx->addr)->sin_port), 2);
			len = 10;
		}
		else
		{
			buf[3] = 0x04;
			memcpy(buf + 4, &(((struct sockaddr_in6 *)ctx->addr)->sin6_addr), 16);
			memcpy(buf + 20, &(((struct sockaddr_in6 *)ctx->addr)->sin6_port), 2);
			len = 22;
		}
		ctx->state = REQ_SENT;
		break;
	}
	default:
	{
		// 不应该来到这里
		assert(0 != 0);
		break;
	}
	}

	ssize_t n = send(ctx->sock, buf, len, 0);
	if (n != len)
	{
		if (n < 0)
		{
			ERROR("send");
		}
		close(ctx->sock);
		(ctx->cb)(-1, ctx->data);
		free(ctx);
		return;
	}

	ev_io_start(EV_A_ &ctx->w_read);
}

static void socks5_recv_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)(w->data);

	UNUSED(revents);
	assert(ctx != NULL);

	ev_io_stop(EV_A_ w);

	uint8_t buf[263];
	bzero(buf, 263);
	ssize_t n = recv(ctx->sock, buf, 263, 0);
	if (n <= 0)
	{
		if (n < 0)
		{
			ERROR("recv");
		}
		close(ctx->sock);
		(ctx->cb)(-1, ctx->data);
		free(ctx);
		return;
	}

	switch (ctx->state)
	{
	case HELLO_SENT:
	{
		if ((buf[0] != 0x05) || (buf[1] != 0x00))
		{
			LOG("SOCKS5 handshake failed");
			close(ctx->sock);
			(ctx->cb)(-1, ctx->data);
			free(ctx);
			return;
		}
		ctx->state = HELLO_RCVD;
		break;
	}
	case REQ_SENT:
	{
		if ((buf[0] != 0x05) || (buf[1] != 0x00))
		{
			LOG("SOCKS5 handshake failed");
			close(ctx->sock);
			(ctx->cb)(-1, ctx->data);
			free(ctx);
			return;
		}
		// 连接建立
		(ctx->cb)(ctx->sock, ctx->data);
		free(ctx);
		return;
	}
	default:
	{
		// 不应该来到这里
		assert(0 != 0);
		break;
	}
	}

	ev_io_start(EV_A_ &ctx->w_write);
}
