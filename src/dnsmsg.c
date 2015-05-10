/*
 * dnsmsg.c - DNS message I/O
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __MINGW32__
#  include "win.h"
#else
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <sys/socket.h>
#endif

#include "dnsmsg.h"
#include "ev.h"
#include "global.h"
#include "log.h"
#include "query.h"

#define UNUSED(x) do {(void)(x);} while (0)

typedef struct
{
	// common
	void *data;
	void (*cb)();
	ev_io w;
	// used in TCP mode
	int msglen;
	int offset;
} ctx_t;

static void query_udp_recv_cb(EV_P_ ev_io *w, int revents);
static void query_tcp_recv_cb(EV_P_ ev_io *w, int revents);
static void reply_udp_recv_cb(EV_P_ ev_io *w, int revents);
static void reply_tcp_recv_cb(EV_P_ ev_io *w, int revents);
static void query_tcp_send_cb(EV_P_ ev_io *w, int revents);
static void reply_tcp_send_cb(EV_P_ ev_io *w, int revents);

void query_recv(int sock, proto_t proto, void (*cb)(uint16_t id))
{
	assert(sock > 0);
	assert(proto == udp || proto == tcp);
	assert(cb != NULL);

	ctx_t *ctx = (ctx_t *)malloc(sizeof(ctx_t));
	if (ctx == NULL)
	{
		LOG("out of memory");
		return;
	}
	ctx->cb = (void (*)())cb;

	if (proto == udp)
	{
		ev_io_init(&ctx->w, query_udp_recv_cb, sock, EV_READ);
	}
	else
	{
		ctx->data = malloc(sizeof(query_t));
		if (ctx->data == NULL)
		{
			LOG("out of memory");
			free(ctx);
			return;
		}
		query_t *query = (query_t *)(ctx->data);
		query->sock = sock;
		query->proto = proto;
		query->state = 0;
		query->msglen = 0;
		ctx->offset = 0;
		ev_io_init(&ctx->w, query_tcp_recv_cb, sock, EV_READ);
	}
	ctx->w.data = (void *)ctx;
	ev_io_start(g_loop, &ctx->w);
}

static void query_udp_recv_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)(w->data);

	UNUSED(loop);
	UNUSED(revents);
	assert(ctx != NULL);
	assert(ctx->data != 0);

	query_t *query = (query_t *)malloc(sizeof(query_t));
	if (query == NULL)
	{
		LOG("out of memory");
		return;
	}
	query->sock = w->fd;
	query->proto = udp;
	query->state = 0;

	query->addrlen = sizeof(struct sockaddr_storage);
	query->msglen = recvfrom(query->sock, query->msg, BUF_SIZE, 0,
		                            (struct sockaddr *)&(query->addr),
		                            &(query->addrlen));
	if (query->msglen <= 0)
	{
		ERROR("recvfrom");
		free(query);
		return;
	}
	if (query_add(query) == 0)
	{
		(ctx->cb)(query->id);
	}
	else
	{
		LOG("query queue full, drop");
	}
}

static void query_tcp_recv_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)(w->data);

	UNUSED(revents);
	assert(ctx != NULL);
	assert(ctx->data != 0);

	query_t *query = (query_t *)(ctx->data);

	if (query->msglen == 0)
	{
		uint16_t msglen;
		ssize_t n = recv(w->fd, &msglen, 2, 0);
		if (n != 2)
		{
			// recv 出错
			if (n < 0)
			{
				ERROR("recv");
			}
			ev_io_stop(EV_A_ w);
			close(w->fd);
			free(query);
			free(ctx);
			return;
		}
		query->msglen = (int)ntohs(msglen);
		if (query->msglen > BUF_SIZE)
		{
			query->msglen = BUF_SIZE;
		}
	}
	else
	{
		ssize_t n = recv(w->fd, query->msg + ctx->offset,
		                 BUF_SIZE - ctx->offset, 0);
		if (n <= 0)
		{
			// recv 出错/连接断开
			if (n < 0)
			{
				ERROR("recv");
			}
			ev_io_stop(EV_A_ w);
			close(w->fd);
			free(query);
			free(ctx);
		}
		else if (ctx->offset + n == ctx->msglen)
		{
			// 读取 DNS 请求完毕
			ev_io_stop(EV_A_ w);
			if (query_add(query) == 0)
			{
				(ctx->cb)(query->id);
			}
			else
			{
				LOG("query queue full, drop");
			}
			free(ctx);
		}
	}
}

void reply_recv(int sock, proto_t proto, void (*cb)(void *msg, int msglen))
{
	assert(sock > 0);
	assert(cb != NULL);
	assert(proto == udp || proto == tcp);

	ctx_t *ctx = (ctx_t *)malloc(sizeof(ctx_t));
	if (ctx == NULL)
	{
		LOG("out of memory");
		return;
	}
	ctx->cb = (void (*)())cb;

	if (proto == udp)
	{
		ev_io_init(&ctx->w, reply_udp_recv_cb, sock, EV_READ);
	}
	else
	{
		ctx->data = malloc(BUF_SIZE);
		if (ctx->data == NULL)
		{
			LOG("out of memory");
			free(ctx);
			return;
		}
		ev_io_init(&ctx->w, reply_tcp_recv_cb, sock, EV_READ);
		ctx->msglen = 0;
		ctx->offset = 0;
	}
	ctx->w.data = (void *)ctx;
	ev_io_start(g_loop, &ctx->w);
}

static void reply_udp_recv_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)(w->data);

	UNUSED(loop);
	UNUSED(revents);
	assert(ctx != NULL);

	void *msg = malloc(BUF_SIZE);
	if (msg == NULL)
	{
		LOG("out of memory");
		return;
	}
	int msglen = recvfrom(w->fd, msg, BUF_SIZE, 0,
	                      NULL, NULL);
	if (msglen <= 0)
	{
		ERROR("recvfrom");
		free(msg);
		return;
	}
	(ctx->cb)(msg, msglen);
}

static void reply_tcp_recv_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)(w->data);

	UNUSED(revents);
	assert(ctx != NULL);
	assert(ctx->data != 0);

	void *msg = ctx->data;

	if (ctx->msglen == 0)
	{
		uint16_t msglen;
		ssize_t n = recv(w->fd, &msglen, 2, 0);
		if (n != 2)
		{
			// recv 出错
			if (n < 0)
			{
				ERROR("recv");
			}
			ev_io_stop(EV_A_ w);
			close(w->fd);
			free(msg);
			free(ctx);
			return;
		}
		ctx->msglen = (int)ntohs(msglen);
		if (ctx->msglen > BUF_SIZE)
		{
			ctx->msglen = BUF_SIZE;
		}
		ctx->offset = 0;
	}
	else
	{
		ssize_t n = recv(w->fd, msg + ctx->offset,
		                 BUF_SIZE - ctx->offset, 0);
		if (n <= 0)
		{
			// recv 出错/连接断开
			if (n < 0)
			{
				ERROR("recv");
			}
			ev_io_stop(EV_A_ w);
			close(w->fd);
			free(msg);
			free(ctx);
		}
		else if (ctx->offset + n == ctx->msglen)
		{
			// 读取 DNS 应答完毕
			ev_io_stop(EV_A_ w);
			close(w->fd);
			(ctx->cb)(msg, ctx->msglen);
			free(ctx);
		}
	}
}

void query_send(int sock, proto_t proto, void *msg, int msglen,
                const struct sockaddr *addr, socklen_t addrlen)
{
	if (proto == udp)
	{
		ssize_t n = sendto(sock, msg, msglen, 0, addr, addrlen);
		if (n < 0)
		{
			ERROR("sendto");
		}
	}
	else
	{
		ctx_t *ctx = (ctx_t *)malloc(sizeof(ctx_t));
		if (ctx == NULL)
		{
			LOG("out of memory");
			return;
		}
		ctx->data = msg;
		ctx->msglen = msglen;
		ctx->offset = -2;
		ev_io_init(&ctx->w, query_tcp_send_cb, sock, EV_WRITE);
		ctx->w.data = (void *)ctx;
		ev_io_start(g_loop, &ctx->w);
	}
}

static void query_tcp_send_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)w->data;

	UNUSED(revents);
	assert(ctx != NULL);
	assert(ctx->data != NULL);

	if (ctx->offset == -2)
	{
		uint16_t msglen = htons((uint16_t)ctx->msglen);
		ssize_t n = send(w->fd, &msglen, 2, 0);
		if (n != 2)
		{
			// send 出错
			if (n <= 0)
			{
				ERROR("send");
			}
			ev_io_stop(EV_A_ w);
			free(ctx);
			return;
		}
		ctx->offset = 0;
	}
	else
	{
		ssize_t n = send(w->fd, ctx->data + ctx->offset,
		                 ctx->msglen - ctx->offset, 0);
		if (n <= 0)
		{
			// send 出错
			if (n < 0)
			{
				ERROR("send");
			}
			ev_io_stop(EV_A_ w);
			close(w->fd);
			free(ctx);
			return;
		}
		if (ctx->offset + n == ctx->msglen)
		{
			// 发送 DNS 消息完毕
			ev_io_stop(EV_A_ w);
		}
	}
}

void reply_send(int sock, proto_t proto, void *msg, int msglen,
                const struct sockaddr *addr, socklen_t addrlen)
{
	if (proto == udp)
	{
		ssize_t n = sendto(sock, msg, msglen, 0, addr, addrlen);
		if (n < 0)
		{
			ERROR("sendto");
		}
		free(msg);
	}
	else
	{
		ctx_t *ctx = (ctx_t *)malloc(sizeof(ctx_t));
		if (ctx == NULL)
		{
			LOG("out of memory");
			return;
		}
		ctx->data = msg;
		ctx->msglen = msglen;
		ctx->offset = -2;
		ev_io_init(&ctx->w, reply_tcp_send_cb, sock, EV_WRITE);
		ctx->w.data = (void *)ctx;
		ev_io_start(g_loop, &ctx->w);
	}
}


static void reply_tcp_send_cb(EV_P_ ev_io *w, int revents)
{
	ctx_t *ctx = (ctx_t *)w->data;

	UNUSED(revents);
	assert(ctx != NULL);
	assert(ctx->data != NULL);

	if (ctx->offset == -2)
	{
		uint16_t msglen = htons((uint16_t)ctx->msglen);
		ssize_t n = send(w->fd, &msglen, 2, 0);
		if (n != 2)
		{
			// send 出错
			if (n <= 0)
			{
				ERROR("send");
			}
			ev_io_stop(EV_A_ w);
			close(w->fd);
			free(ctx->data);
			free(ctx);
			return;
		}
		ctx->offset = 0;
	}
	else
	{
		ssize_t n = send(w->fd, ctx->data + ctx->offset,
		                 ctx->msglen - ctx->offset, 0);
		if (n <= 0)
		{
			// send 出错
			if (n < 0)
			{
				ERROR("send");
			}
			ev_io_stop(EV_A_ w);
			close(w->fd);
			free(ctx->data);
			free(ctx);
			return;
		}
		if (ctx->offset + n == ctx->msglen)
		{
			// 发送 DNS 消息完毕
			ev_io_stop(EV_A_ w);
			close(w->fd);
			free(ctx->data);
			free(ctx);
		}
	}
}
