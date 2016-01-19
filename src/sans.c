/*
 * sans.c - simple anti-poultion name server
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
#include "cache.h"
#include "conf.h"
#include "dns.h"
#include "dnsmsg.h"
#include "event.h"
#include "log.h"
#include "query.h"
#include "sans.h"
#include "utils.h"


/*
 * @var  verbose
 * @desc print verbose message or not
 */
static int verbose;

/*
 * @var  nspresolver
 * @desc use NoStandardPort UDP Resolver or not
 */
static int nspresolver;

/*
 * @var  socks5
 * @desc connect via SOCKS5 or not
 */
static int socks5;


/*
 * @desc socket file descriptor
 */
static int sock_udp;
static int sock_tcp;
static int sock_server;
static int sock_cn;
static int sock_test;


/*
 * @desc sockaddr of upstream servers
 */
static struct
{
    struct sockaddr_storage addr;
    socklen_t addrlen;
} test_server, cn_server, server;


static void tick_cb(void);
static void accept_cb(ev_io *w);
static void query_cb(uint16_t id);
static void test_cb(void *msg, int msglen);
static void connect_cb(int sock, void *data);
static void reply_cb(void *msg, int msglen);


/*
 * @func sans_init()
 * @desc initialize
 */
int sans_init(const conf_t *conf)
{
#ifdef __MINGW32__
    // initialize winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        LOG("failed to initialize winsock");
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        WSACleanup();
        LOG("failed to find a usable version of winsock");
    }
#endif

    verbose = conf->verbose;
    nspresolver = conf->nspresolver;

    struct addrinfo hints;
    struct addrinfo *res;

    // 初始化 event loop
    ev_init(tick_cb);

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
    if (bind(sock_udp, res->ai_addr, (int)(res->ai_addrlen)) != 0)
    {
        ERROR("bind");
        freeaddrinfo(res);
        return -1;
    }
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

#ifdef __MINGW32__
    // fix weird bug with winsock
    const unsigned int SIO_UDP_CONNRESET = 0x9800000cU;
    int no_connset = 0;
    int wsa_ret;
    WSAIoctl(sock_udp, SIO_UDP_CONNRESET, &no_connset, sizeof(no_connset),
             NULL, 0, &wsa_ret, NULL, NULL);
    WSAIoctl(sock_tcp, SIO_UDP_CONNRESET, &no_connset, sizeof(no_connset),
             NULL, 0, &wsa_ret, NULL, NULL);
    WSAIoctl(sock_server, SIO_UDP_CONNRESET, &no_connset, sizeof(no_connset),
             NULL, 0, &wsa_ret, NULL, NULL);
    WSAIoctl(sock_cn, SIO_UDP_CONNRESET, &no_connset, sizeof(no_connset),
             NULL, 0, &wsa_ret, NULL, NULL);
    WSAIoctl(sock_test, SIO_UDP_CONNRESET, &no_connset, sizeof(no_connset),
             NULL, 0, &wsa_ret, NULL, NULL);
#endif

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


/*
 * @func sans_run()
 */
int sans_run(void)
{
    // 处理 TCP 连接请求
    ev_io w_tcp;
    ev_io_init(&w_tcp, accept_cb, sock_tcp, EV_READ);
    ev_io_start(&w_tcp);

    query_recv(sock_udp, ns_udp, query_cb);
    reply_recv(sock_test, ns_udp, test_cb);
    reply_recv(sock_cn, ns_udp, reply_cb);
    reply_recv(sock_server, ns_udp, reply_cb);

    // 开始事件循环
    ev_run();

    // 清理
    close(sock_tcp);
    close(sock_udp);
    close(sock_test);
    close(sock_cn);
    close(sock_server);
#ifdef __MINGW32__
    WSACleanup();
#endif
    LOG("exit");

    return EXIT_SUCCESS;
}


/*
 * @func sans_stop()
 */
void sans_stop(void)
{
    ev_stop();
}


/*
 * @func tick_cb()
 * @desc tick every seconds
 */
static void tick_cb(void)
{
    query_tick();
    cache_tick();
}


/*
 * @func accept_cb()
 * @desc local TCP accept callback
 */
static void accept_cb(ev_io *w)
{
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

    query_recv(sock, ns_tcp, query_cb);
}


/*
 * @func query_cb()
 * @desc callback to handle DNS query
 */
static void query_cb(uint16_t id)
{
    query_t *query = query_search(id);
    assert(query != NULL);

    if (verbose)
    {
        LOG("query [%u] [%s] [%s]", query->id, ns_type_str(query->type), query->name);
    }

    // 使用新 ID
    query->id = ns_newid();

    // 在 cache 中查找域名是否被污染
    cache_t *cache = cache_search(query->name, ns_t_block);

    if (cache == NULL)
    {
        // 查询 SOA 记录，以判断域名是否被污染
        if (verbose)
        {
            LOG("detect [%s]", query->name);
        }
        uint8_t msg[NS_PACKETSZ];
        int msglen = ns_mkquery(msg, NS_PACKETSZ, query->name, ns_t_soa);
        ns_setid(msg, query->id);
        query_send(sock_test, ns_udp, msg, msglen,
                   (struct sockaddr *)&test_server.addr, test_server.addrlen);
    }
    else if (*(ns_block *)(cache->data))
    {
        // 被污染的域名
        if (nspresolver)
        {
            uint8_t msg[NS_PACKETSZ];
            int msglen = ns_mkquery(msg, NS_PACKETSZ, query->name, query->type);
            ns_setid(msg, query->id);
            query_send(sock_server, ns_udp, msg, msglen,
                       (struct sockaddr *)&server.addr, server.addrlen);
        }
        else
        {
            async_connect((struct sockaddr *)&(server.addr), server.addrlen,
                        connect_cb, socks5, (void *)(uintptr_t)(query->id));
        }
    }
    else
    {
        // 域名没被污染
        uint8_t msg[NS_PACKETSZ];
        int msglen = ns_mkquery(msg, NS_PACKETSZ, query->name, query->type);
        ns_setid(msg, query->id);
        query_send(sock_cn, ns_udp, msg, msglen,
                   (struct sockaddr *)&cn_server.addr, cn_server.addrlen);
    }
}


/*
 * @func test_cb()
 * @desc callback to handle test reply
 */
static void test_cb(void *msg, int msglen)
{
    query_t *query = query_search(ns_getid(msg));
    if (query == NULL)
    {
        return;
    }

    // 使用新 ID
    query->id = ns_newid();

    char name[NS_NAMESZ];
    int type = ns_t_invalid;
    if (ns_parse_reply(msg, msglen, name, &type) != 0)
    {
        LOG("bad reply");
        return;
    }

    cache_t *cache = (cache_t *)malloc(sizeof(cache_t) + sizeof(ns_block));
    if (cache == NULL)
    {
        LOG("out of memory");
        return;
    }
    strcpy(cache->name, name);
    cache->ttl = 518400U;
    cache->type = ns_t_block;

    if (type == ns_t_a)
    {
        // 查询 SOA 记录却返回 A 记录，说明域名被污染了
        if (verbose)
        {
            LOG("[%s] is blocked", name);
        }
        if (nspresolver)
        {
            *(ns_block *)(cache->data) = 1;
            uint8_t msg2[NS_PACKETSZ];
            int msglen2 = ns_mkquery(msg2, NS_PACKETSZ, query->name, query->type);
            ns_setid(msg2, query->id);
            query_send(sock_server, ns_udp, msg2, msglen2,
                       (struct sockaddr *)&server.addr,
                       server.addrlen);
        }
        else
        {
            *(ns_block *)(cache->data) = 1;
            async_connect((struct sockaddr *)&(server.addr), server.addrlen,
                        connect_cb, socks5, (void *)(uintptr_t)(query->id));
        }
    }
    else
    {
        // 域名未被污染
        if (verbose)
        {
            LOG("[%s] is not blocked", name);
        }
        *(ns_block *)(cache->data) = 0;
        uint8_t msg2[NS_PACKETSZ];
        int msglen2 = ns_mkquery(msg2, NS_PACKETSZ, query->name, query->type);
        ns_setid(msg2, query->id);
        query_send(sock_cn, ns_udp, msg2, msglen2,
                   (struct sockaddr *)&cn_server.addr,
                   cn_server.addrlen);
    }
    cache_insert(cache);
}


/*
 * @func  connect_cb()
 * @desc  TCP/SOCKS5 connect callback
 * @param sock - fd
 *        data - query ID
 */
static void connect_cb(int sock, void *data)
{
    query_t *query = query_search((uint16_t)(uintptr_t)(data));

    assert(query != NULL);

    if (sock < 0)
    {
        query_delete(query->id);
        return;
    }

    uint8_t msg[NS_PACKETSZ];
    int msglen = ns_mkquery(msg, NS_PACKETSZ, query->name, query->type);
    ns_setid(msg, query->id);
    query_send(sock, ns_tcp, msg, msglen, NULL, 0);
    reply_recv(sock, ns_tcp, reply_cb);
}


/*
 * @func reply_cb()
 * @desc callback to handle DNS reply
 */
static void reply_cb(void *msg, int msglen)
{
    if (verbose)
    {
        uint16_t id = ns_getid(msg);
        char name[NS_NAMESZ];
        int type = ns_t_invalid;
        if (ns_parse_reply(msg, msglen, name, &type) != 0)
        {
            LOG("bad reply");
            return;
        }
        LOG("reply [%u] [%s] [%s]", id, ns_type_str(type), name);
    }

    query_t *query = query_search(ns_getid(msg));
    if (query == NULL)
    {
        return;
    }

    ns_setid(msg, query->qid);
    if (query->sock > 0)
    {
        reply_send(query->sock, query->protocol, msg, msglen,
                   (struct sockaddr *)&(query->addr), query->addrlen);
    }
    query_delete(query->id);
}
