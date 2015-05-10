/*
 * sans.c - Simple anti-poultion name server
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

#include "async_connect.h"
#include "conf.h"
#include "db.h"
#include "dname.h"
#include "dns.h"
#include "dnsmsg.h"
#include "ev.h"
#include "global.h"
#include "log.h"
#include "query.h"
#include "sans.h"
#include "socks5.h"
#include "utils.h"

#define UNUSED(x) do {(void)(x);} while (0)

static void accept_cb(EV_P_ ev_io *w, int revents);
static void query_cb(uint16_t id);
static void test_cb(void *msg, int msglen);
static void reply_cb(void *msg, int msglen);

static int verbose;
static int socks5;

static int sock_udp;
static int sock_tcp;
static int sock_server;
static int sock_cn;
static int sock_test;

static struct
{
	struct sockaddr_storage addr;
	socklen_t addrlen;
} test_server, cn_server, server;

// 初始化
int sans_init(const conf_t *conf)
{
	verbose = conf->verbose;

	struct addrinfo hints;
	struct addrinfo *res;

#ifdef __MINGW32__
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		LOG("Could not initialize winsock");
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		LOG("Could not find a usable version of winsock");
	}
#endif

	// 初始化 libev
	g_loop = ev_default_loop(0);
	if (g_loop == NULL)
	{
		LOG("failed to initialize libev");
		return -1;
	}

	// 解析 test_server 地址
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	if (getaddrinfo(conf->test_server.addr, conf->test_server.port, &hints, &res) != 0)
	{
		ERROR("getaddrinfo");
		return -1;
	}
	memcpy(&test_server.addr, res->ai_addr, res->ai_addrlen);
	test_server.addrlen = res->ai_addrlen;
	freeaddrinfo(res);

	// 解析 cn_server 地址
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	if (getaddrinfo(conf->cn_server.addr, conf->cn_server.port, &hints, &res) != 0)
	{
		ERROR("getaddrinfo");
		return -1;
	}
	memcpy(&cn_server.addr, res->ai_addr, res->ai_addrlen);
	cn_server.addrlen = res->ai_addrlen;
	freeaddrinfo(res);

	// 解析 server 地址
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	if (getaddrinfo(conf->server.addr, conf->server.port, &hints, &res) != 0)
	{
		ERROR("getaddrinfo");
		return -1;
	}
	memcpy(&server.addr, res->ai_addr, res->ai_addrlen);
	server.addrlen = res->ai_addrlen;
	freeaddrinfo(res);

	// 初始化本地监听 TCP socket
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(conf->listen.addr, conf->listen.port, &hints, &res) != 0)
	{
		ERROR("getaddrinfo");
		return -1;
	}
	sock_tcp = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock_tcp < 0)
	{
		ERROR("socket");
		freeaddrinfo(res);
		return -1;
	}
	setreuseaddr(sock_tcp);
	if (bind(sock_tcp, res->ai_addr, (int)(res->ai_addrlen)) != 0)
	{
		ERROR("bind");
		freeaddrinfo(res);
		return -1;
	}
	freeaddrinfo(res);
	if (listen(sock_tcp, SOMAXCONN) != 0)
	{
		ERROR("listen");
		return -1;
	}
	setnonblock(sock_tcp);
#ifdef SO_NOSIGPIPE
	setnosigpipe(sock_tcp);
#endif

	// 初始化本地监听 UDP socket
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(conf->listen.addr, conf->listen.port, &hints, &res) != 0)
	{
		ERROR("getaddrinfo");
		return -1;
	}
	sock_udp = socket(res->ai_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_udp < 0)
	{
		ERROR("socket");
		freeaddrinfo(res);
		return -1;
	}
	setnonblock(sock_udp);
	setreuseaddr(sock_udp);
	if (bind(sock_udp, res->ai_addr, (int)(res->ai_addrlen)) != 0)
	{
		ERROR("bind");
		freeaddrinfo(res);
		return -1;
	}
	freeaddrinfo(res);

	// 初始化 remote UDP socket
	sock_test = socket(test_server.addr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_test < 0)
	{
		ERROR("socket");
		return -1;
	}
	setnonblock(sock_test);
	sock_cn = socket(cn_server.addr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_cn < 0)
	{
		ERROR("socket");
		return -1;
	}
	setnonblock(sock_cn);
	sock_server = socket(server.addr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_server < 0)
	{
		ERROR("socket");
		return -1;
	}
	setnonblock(sock_server);

	// 初始化 SOCKS5
	if (conf->socks5.addr[0] == '\0')
	{
		socks5 = 0;
	}
	else
	{
		if (socks5_init(conf->socks5.addr, conf->socks5.port) == 0)
		{
			socks5 = 1;
		}
		else
		{
			socks5 = 0;
		}
	}

	// 初始化 dname
	if (dname_init() != 0)
	{
		return -1;
	}

	// drop root privilege
	if (conf->user[0] != '\0')
	{
		if (runas(conf->user) != 0)
		{
			ERROR("runas");
		}
	}

	LOG("starting sans at %s:%s", conf->listen.addr, conf->listen.port);

	return 0;
}

// 运行
int sans_run(void)
{
#ifndef __MINGW32__
	// There's an unknown bug while building with MinGW
	ev_io w_tcp;
	ev_io_init(&w_tcp, accept_cb, sock_tcp, EV_READ);
	ev_io_start(g_loop, &w_tcp);
#endif

	query_recv(sock_udp, udp, query_cb);
	reply_recv(sock_test, udp, test_cb);
	reply_recv(sock_cn, udp, reply_cb);
	reply_recv(sock_server, udp, reply_cb);

	// 开始事件循环
	ev_run(g_loop, 0);

	// 清理
	close(sock_tcp);
	close(sock_udp);
	close(sock_test);
	close(sock_cn);
	close(sock_server);
#ifdef __MINGW32__
	WSACleanup();
#endif
	LOG("Exit");

	return EXIT_SUCCESS;
}

static void accept_cb(EV_P_ ev_io *w, int revents)
{
	UNUSED(loop);
	UNUSED(revents);

	int sock = accept(w->fd, NULL, NULL);
	if (sock < 0)
	{
		ERROR("accept");
		return;
	}
	setnonblock(sock);
	settimeout(sock);
	setkeepalive(sock);
#ifdef SO_NOSIGPIPE
	setnosigpipe(sock);
#endif

	query_recv(sock, tcp, query_cb);
}

static void connect_cb(int sock, void *data)
{
	query_t *query = query_search((uint16_t)(uintptr_t)(data));

	assert(query != NULL);

	if (sock < 0)
	{
		query_del(query->id);
		return;
	}

	query_send(sock, tcp, query->msg, query->msglen, NULL, 0);
	reply_recv(sock, tcp, reply_cb);
}

static void test_cb(void *msg, int msglen)
{
	query_t *query = query_search(ns_getid(msg));
	if (query == NULL)
	{
		free(msg);
		return;
	}

	char *dname;
	int qtype = 0;
	if (ns_parse_reply(msg, msglen, &dname, &qtype) != 0)
	{
		LOG("bad reply");
		free(msg);
		return;
	}
	free(msg);

	query_newid(query);

	if (qtype == ns_t_a)
	{
		db_insert(dname, 2);
		if (socks5)
		{
			socks5_connect((struct sockaddr *)&server.addr, server.addrlen,
			               connect_cb, (void *)(uintptr_t)query->id);
		}
		else
		{
			async_connect((struct sockaddr *)&server.addr, server.addrlen,
			              connect_cb, (void *)(uintptr_t)query->id);
		}
	}
	else
	{
		db_insert(dname, 1);
		query_send(sock_cn, udp, query->msg, query->msglen,
		           (struct sockaddr *)&cn_server.addr,
		           cn_server.addrlen);
	}
	dname_free(dname);
}

static void reply_cb(void *msg, int msglen)
{
	query_t *query = query_search(ns_getid(msg));
	if (query == NULL)
	{
		free(msg);
		return;
	}

	ns_setid(msg, query->ori_id);
	reply_send(query->sock, query->proto, msg, msglen,
	           (struct sockaddr *)&(query->addr), query->addrlen);
	query_del(query->id);
}

static void query_cb(uint16_t id)
{
	query_t *query = query_search(id);
	assert(query != NULL);

	char *dname;
	int qtype;
	if (ns_parse_query(query->msg, query->msglen, &dname, &qtype) != 0)
	{
		LOG("bad query");
		query_del(id);
		return;
	}

	if (verbose)
	{
		LOG("query %s", dname);
	}
	int flag;
	if (qtype == ns_t_ptr)
	{
		flag = 1;
	}
	else
	{
		flag = db_search(dname);
	}
	query_newid(query);
	if (flag == 0)
	{
		// 查询 SOA 记录，以判断域名是否被污染
		void *msg2 = malloc(BUF_SIZE);
		if (msg2 == NULL)
		{
			LOG("out of memory");
			query_del(id);
			return;
		}
		int msglen = ns_mkquery(msg2, BUF_SIZE, dname, ns_t_soa);
		if (msglen < 0)
		{
			LOG("res_mkquery error");
			free(msg2);
			query_del(id);
			return;
		}
		ns_setid(msg2, query->id);
		query_send(sock_test, udp, msg2, msglen,
		           (struct sockaddr *)&test_server.addr, test_server.addrlen);
	}
	else if (flag == 1)
	{
		query_send(sock_cn, udp, query->msg, query->msglen,
		           (struct sockaddr *)&cn_server.addr, cn_server.addrlen);
	}
	else if (flag == 2)
	{
		if (socks5)
		{
			socks5_connect((struct sockaddr *)&(server.addr), server.addrlen,
			               connect_cb, (void *)(uintptr_t)id);
		}
		else
		{
			async_connect((struct sockaddr *)&(server.addr), server.addrlen,
			              connect_cb, (void *)(uintptr_t)id);
		}
	}
	dname_free(dname);
}

static void stop_cb(EV_P_ ev_async *w, int revents)
{
	UNUSED(revents);

	ev_async_stop(EV_A_ w);
	ev_break(EV_A_ EVBREAK_ALL);
}

// 停止
void sans_stop(void)
{
	static ev_async w_stop;
	ev_async_init(&w_stop, stop_cb);
	ev_async_start(g_loop, &w_stop);
	ev_async_send(g_loop, &w_stop);
}
