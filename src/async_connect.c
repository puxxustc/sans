/*
 * async_connect.c - async connect
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

#include "async_connect.h"
#include "event.h"
#include "log.h"
#include "utils.h"


/*
 *  @desc SOCKS5 connection state
 */
enum
{
    CLOSED = 0,
    HELLO_SENT,
    HELLO_RCVD,
    REQ_SENT,
    ESTAB
};


/*
 * @type ctx_t
 * @desc connection context
 */
typedef struct
{
    int state;
    int socks5;
    const struct sockaddr *addr;
    socklen_t addrlen;
    void (*cb)(int, void *);
    void *data;
    ev_io w_read;
    ev_io w_write;
} ctx_t;


static void connect_cb(ev_io *w);
static void socks5_send_cb(ev_io *w);
static void socks5_recv_cb(ev_io *w);


/*
 * @var  server
 * @desc sockaddr of SOCKS5 server
 */
static struct
{
    socklen_t addrlen;
    struct sockaddr_storage addr;
} server;


/*
 * @func  socks5_init()
 * @desc  initialize SOCKS5 server address
 * @param host - host of IP address of SOCKS5 server
 *        port - port of SOCKS5 server
 */
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
    memcpy(&(server.addr), res->ai_addr, res->ai_addrlen);
    server.addrlen = res->ai_addrlen;
    freeaddrinfo(res);

    return 0;
}


/*
 * @func  async_connect()
 * @desc  async connect (support SOCKS5)
 * @param addr    - peer address
 *        addrlen - length of addr
 *        cb      - callback
 *        socks5  - connect via SOCKS5 or not
 *        data    - additional data
 */
void async_connect(const struct sockaddr *addr, socklen_t addrlen,
                   void (*cb)(int, void *), int socks5, void *data)
{
    ctx_t *ctx = (ctx_t *)malloc(sizeof(ctx_t));
    if (ctx == NULL)
    {
        LOG("out of memory");
        (cb)(-1, data);
        return;
    }
    ctx->socks5 = socks5;

    if (socks5)
    {
        // connect via SOCKS5 proxy
        ctx->cb = cb;
        ctx->data = data;
        ctx->addr = addr;
        ctx->addrlen = addrlen;

        int sock = socket(server.addr.ss_family, SOCK_STREAM, IPPROTO_TCP);
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

        if (connect(sock, (struct sockaddr *)&server.addr, server.addrlen) != 0)
        {
            if (errno != EINPROGRESS)
            {
                // 连接失败
                LOG("connect to SOCKS5 server failed");
                close(sock);
                free(ctx);
                (cb)(-1, data);
                return;
            }
        }
        ev_io_init(&(ctx->w_write), connect_cb, sock, EV_WRITE);
        ctx->w_write.data = (void *)ctx;
        ev_io_start(&(ctx->w_write));
    }
    else
    {
        // connect directly
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
        ev_io_init(&(ctx->w_write), connect_cb, sock, EV_WRITE);
        ctx->w_write.data = (void *)ctx;
        ev_io_start(&(ctx->w_write));
    }
}


/*
 * @func connect_cb
 * @desc connect callback
 */
static void connect_cb(ev_io *w)
{
    ctx_t *ctx = (ctx_t *)(w->data);

    assert(ctx != NULL);

    ev_io_stop(w);

    if (getsockerror(w->fd) == 0)
    {
        // 连接成功
        if (ctx->socks5)
        {
            ctx->state = CLOSED;
            ev_io_init(&ctx->w_read, socks5_recv_cb, w->fd, EV_READ);
            ev_io_init(&ctx->w_write, socks5_send_cb, w->fd, EV_WRITE);
            ctx->w_read.data = (void *)ctx;
            ctx->w_write.data = (void *)ctx;
            ev_io_start(&(ctx->w_write));
        }
        else
        {
            (ctx->cb)(w->fd, ctx->data);
            free(ctx);
        }
    }
    else
    {
        // 连接失败
        if (ctx->socks5)
        {
            LOG("connect to SOCKS5 server failed");
            close(w->fd);
            (ctx->cb)(-1, ctx->data);
            free(ctx);
        }
        else
        {
            LOG("connect failed");
            close(w->fd);
            (ctx->cb)(-1, ctx->data);
            free(ctx);
        }
    }
}


/*
 * @func socks5_send_cb()
 * @desc SOCKS5 send callback
 */
static void socks5_send_cb(ev_io *w)
{
    ctx_t *ctx = (ctx_t *)(w->data);

    assert(ctx != NULL);

    ev_io_stop(w);

    uint8_t buf[263];
    ssize_t len = 0;

    switch (ctx->state)
    {
    case CLOSED:
        buf[0] = 0x05;
        buf[1] = 0x01;
        buf[2] = 0x00;
        len = 3;
        ctx->state = HELLO_SENT;
        break;
    case HELLO_RCVD:
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
    default:
        // 不应该来到这里
        assert("bad connection state" == NULL);
        break;
    }

    ssize_t n = send(w->fd, buf, len, 0);
    if (n != len)
    {
        if (n < 0)
        {
            ERROR("send");
        }
        close(w->fd);
        (ctx->cb)(-1, ctx->data);
        free(ctx);
        return;
    }

    ev_io_start(&(ctx->w_read));
}


/*
 * @func socks5_recv_cb()
 * @desc SOCKS5 recv callback
 */
static void socks5_recv_cb(ev_io *w)
{
    ctx_t *ctx = (ctx_t *)(w->data);

    assert(ctx != NULL);

    ev_io_stop(w);

    uint8_t buf[263];
    bzero(buf, 263);
    ssize_t n = recv(w->fd, buf, 263, 0);
    if (n <= 0)
    {
        if (n < 0)
        {
            ERROR("recv");
        }
        close(w->fd);
        (ctx->cb)(-1, ctx->data);
        free(ctx);
        return;
    }

    switch (ctx->state)
    {
    case HELLO_SENT:
        if ((buf[0] != 0x05) || (buf[1] != 0x00))
        {
            LOG("SOCKS5 handshake failed");
            close(w->fd);
            (ctx->cb)(-1, ctx->data);
            free(ctx);
            return;
        }
        ctx->state = HELLO_RCVD;
        break;
    case REQ_SENT:
        if ((buf[0] != 0x05) || (buf[1] != 0x00))
        {
            LOG("SOCKS5 handshake failed");
            close(w->fd);
            (ctx->cb)(-1, ctx->data);
            free(ctx);
            return;
        }
        // 连接建立
        (ctx->cb)(w->fd, ctx->data);
        free(ctx);
        return;
    default:
        // 不应该来到这里
        assert("bad connection state" == NULL);
        break;
    }

    ev_io_start(&(ctx->w_write));
}
