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

#include "dns.h"
#include "dnsmsg.h"
#include "event.h"
#include "log.h"
#include "query.h"


/*
* @type ctx_t
* @desc connection context
*/
typedef struct
{
    // common
    void (*cb)();
    void *msg;
    int protocol;
    ev_io w;
    // used in TCP mode
    int msglen;
    int offset;
} ctx_t;


static void query_udp_recv_cb(ev_io *w);
static void query_tcp_recv_cb(ev_io *w);
static void reply_udp_recv_cb(ev_io *w);
static void reply_tcp_recv_cb(ev_io *w);
static void query_tcp_send_cb(ev_io *w);
static void reply_tcp_send_cb(ev_io *w);


/*
 * @func query_recv()
 * @desc receive DNS query asynchronously
 * @memo msg whill be freed after callback
 */
void query_recv(int sock, int protocol, void (*cb)(uint16_t id))
{
    assert(sock > 0);
    assert(protocol == ns_udp || protocol == ns_tcp);
    assert(cb != NULL);

    ctx_t *ctx = (ctx_t *)malloc(sizeof(ctx_t));
    if (ctx == NULL)
    {
        LOG("out of memory");
        return;
    }
    ctx->cb = (void (*)())cb;

    if (protocol == ns_udp)
    {
        ev_io_init(&(ctx->w), query_udp_recv_cb, sock, EV_READ);
    }
    else
    {
        ctx->msg = malloc(NS_PACKETSZ);
        if (ctx->msg == NULL)
        {
            LOG("out of memory");
            free(ctx);
            return;
        }
        ev_io_init(&(ctx->w), query_tcp_recv_cb, sock, EV_READ);
        ctx->msglen = 0;
        ctx->offset = 0;
    }
    ctx->w.data = (void *)ctx;
    ev_io_start(&(ctx->w));
}


/*
 * @func query_udp_recv_cb()
 * @desc callback for recivie DNS message via UDP
 */
static void query_udp_recv_cb(ev_io *w)
{
    ctx_t *ctx = (ctx_t *)(w->data);

    assert(ctx != NULL);

    uint8_t msg[NS_PACKETSZ];
    query_t *query = (query_t *)malloc(sizeof(query_t));
    if (query == NULL)
    {
        LOG("out of memory");
        return;
    }
    query->sock = w->fd;
    query->protocol = ns_udp;
    query->addrlen = sizeof(struct sockaddr_storage);
    int msglen = recvfrom(w->fd, msg, NS_PACKETSZ, 0,
                          (struct sockaddr *)&(query->addr),
                          &(query->addrlen));
    if (msglen <= 0)
    {
        ERROR("recvfrom");
    }
    else
    {
        query->id = ns_getid(msg);
        if (ns_parse_query(msg, msglen, query->name, &(query->type)) != 0)
        {
            LOG("bad query");
        }
        else
        {
            if (query_add(query) == 0)
            {
                (ctx->cb)(query->id);
            }
            else
            {
                free(query);
            }
        }
    }
}


/*
 * @func query_tcp_recv_cb()
 * @desc callback for recivie DNS query via TCP
 */
static void query_tcp_recv_cb(ev_io *w)
{
    ctx_t *ctx = (ctx_t *)(w->data);

    assert(ctx != NULL);
    assert(ctx->msg != 0);

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
            ev_io_stop(w);
            close(w->fd);
            free(ctx->msg);
            free(ctx);
            return;
        }
        ctx->msglen = (int)ntohs(msglen);
        if (ctx->msglen > NS_PACKETSZ)
        {
            ctx->msglen = NS_PACKETSZ;
        }
        ctx->offset = 0;
    }
    else
    {
        ssize_t n = recv(w->fd, ctx->msg + ctx->offset,
                         NS_PACKETSZ - ctx->offset, 0);
        if (n <= 0)
        {
            // recv 出错/连接断开
            if (n < 0)
            {
                ERROR("recv");
            }
            ev_io_stop(w);
            close(w->fd);
            free(ctx->msg);
            free(ctx);
        }
        else if (ctx->offset + n == ctx->msglen)
        {
            // 读取 DNS 请求完毕
            ev_io_stop(w);
            query_t *query = (query_t *)malloc(sizeof(query_t));
            if (query == NULL)
            {
                LOG("out of memory");
                free(ctx->msg);
                free(ctx);
                return;
            }
            query->sock = w->fd;
            query->protocol = ns_tcp;
            query->id = ns_getid(ctx->msg);
            if (ns_parse_query(ctx->msg, ctx->msglen, query->name, &(query->type)) == 0)
            {
                if (query_add(query) == 0)
                {
                    (ctx->cb)(query->id);
                }
                else
                {
                    free(query);
                }
            }
            else
            {
                LOG("bad query");
            }
            free(ctx->msg);
            free(ctx);
        }
    }
}


/*
 * @func reply_recv()
 * @desc receive DNS query asynchronously
 * @memo msg whill be freed after callback
 */
void reply_recv(int sock, int protocol, void (*cb)(void *msg, int msglen))
{
    assert(sock > 0);
    assert(protocol == ns_udp || protocol == ns_tcp);
    assert(cb != NULL);

    ctx_t *ctx = (ctx_t *)malloc(sizeof(ctx_t));
    if (ctx == NULL)
    {
        LOG("out of memory");
        return;
    }
    ctx->cb = cb;

    if (protocol == ns_udp)
    {
        ev_io_init(&(ctx->w), reply_udp_recv_cb, sock, EV_READ);
    }
    else
    {
        ctx->msg = malloc(NS_PACKETSZ);
        if (ctx->msg == NULL)
        {
            LOG("out of memory");
            free(ctx);
            return;
        }
        ev_io_init(&(ctx->w), reply_tcp_recv_cb, sock, EV_READ);
        ctx->msglen = 0;
        ctx->offset = 0;
    }
    ctx->w.data = (void *)ctx;
    ev_io_start(&(ctx->w));
}


/*
 * @func reply_udp_recv_cb()
 * @desc callback for recivie DNS message via UDP
 */
static void reply_udp_recv_cb(ev_io *w)
{
    ctx_t *ctx = (ctx_t *)(w->data);

    assert(ctx != NULL);

    uint8_t msg[NS_PACKETSZ];
    int msglen = recvfrom(w->fd, msg, NS_PACKETSZ, 0, NULL, NULL);
    if (msglen <= 0)
    {
        ERROR("recvfrom");
    }
    else
    {
        (ctx->cb)(msg, msglen);
    }
}


/*
 * @func reply_tcp_recv_cb()
 * @desc callback for recivie DNS reply via TCP
 */
static void reply_tcp_recv_cb(ev_io *w)
{
    ctx_t *ctx = (ctx_t *)(w->data);

    assert(ctx != NULL);
    assert(ctx->msg != 0);

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
            ev_io_stop(w);
            close(w->fd);
            free(ctx->msg);
            free(ctx);
            return;
        }
        ctx->msglen = (int)ntohs(msglen);
        if (ctx->msglen > NS_PACKETSZ)
        {
            ctx->msglen = NS_PACKETSZ;
        }
        ctx->offset = 0;
    }
    else
    {
        ssize_t n = recv(w->fd, ctx->msg + ctx->offset,
                         NS_PACKETSZ - ctx->offset, 0);
        if (n <= 0)
        {
            // recv 出错/连接断开
            if (n < 0)
            {
                ERROR("recv");
            }
            ev_io_stop(w);
            close(w->fd);
            free(ctx->msg);
            free(ctx);
        }
        else if (ctx->offset + n == ctx->msglen)
        {
            // 读取 DNS 应答完毕
            ev_io_stop(w);
            close(w->fd);
            (ctx->cb)(ctx->msg, ctx->msglen);
            free(ctx->msg);
            free(ctx);
        }
    }
}


/*
 * @func query_send()
 * @desc send DNS query
 * @memo synchronously for UDP, asynchronously for TCP
 */
void query_send(int sock, int protocol, void *msg, int msglen,
                const struct sockaddr *addr, socklen_t addrlen)
{
    if (protocol == ns_udp)
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
        ctx->msg = malloc(msglen);
        if (ctx->msg == NULL)
        {
            LOG("out of memory");
            free(ctx);
            return;
        }
        memcpy(ctx->msg, msg, msglen);
        ctx->msglen = msglen;
        ctx->offset = -2;
        ev_io_init(&(ctx->w), query_tcp_send_cb, sock, EV_WRITE);
        ctx->w.data = (void *)ctx;
        ev_io_start(&(ctx->w));
    }
}


/*
 * @func query_tcp_send_cb()
 * @desc callback for send DNS query via TCP
 */
static void query_tcp_send_cb(ev_io *w)
{
    ctx_t *ctx = (ctx_t *)w->data;

    assert(ctx != NULL);
    assert(ctx->msg != NULL);

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
            ev_io_stop(w);
            free(ctx->msg);
            free(ctx);
            return;
        }
        ctx->offset = 0;
    }
    else
    {
        ssize_t n = send(w->fd, ctx->msg + ctx->offset,
                         ctx->msglen - ctx->offset, 0);
        if (n <= 0)
        {
            // send 出错
            if (n < 0)
            {
                ERROR("send");
            }
            ev_io_stop(w);
            close(w->fd);
            free(ctx->msg);
            free(ctx);
            return;
        }
        if (ctx->offset + n == ctx->msglen)
        {
            // 发送 DNS 请求完毕
            ev_io_stop(w);
            free(ctx->msg);
            free(ctx);
        }
    }
}


/*
 * @func reply_send()
 * @desc send DNS reply
 * @memo synchronously for UDP, asynchronously for TCP
 *       if protocol is TCP, sock will be closed after reply sent
 */
void reply_send(int sock, int protocol, void *msg, int msglen,
                const struct sockaddr *addr, socklen_t addrlen)
{
    if (protocol == ns_udp)
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
        ctx->msg = malloc(msglen);
        if (ctx->msg == NULL)
        {
            LOG("out of memory");
            free(ctx);
            return;
        }
        memcpy(ctx->msg, msg, msglen);
        ctx->msglen = msglen;
        ctx->offset = -2;
        ev_io_init(&(ctx->w), reply_tcp_send_cb, sock, EV_WRITE);
        ctx->w.data = (void *)ctx;
        ev_io_start(&(ctx->w));
    }
}


/*
 * @func reply_tcp_send_cb()
 * @desc callback for send DNS reply via TCP
 */
static void reply_tcp_send_cb(ev_io *w)
{
    ctx_t *ctx = (ctx_t *)(w->data);

    assert(ctx != NULL);
    assert(ctx->msg != NULL);

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
            ev_io_stop(w);
            close(w->fd);
            free(ctx->msg);
            free(ctx);
            return;
        }
        ctx->offset = 0;
    }
    else
    {
        ssize_t n = send(w->fd, ctx->msg + ctx->offset,
                         ctx->msglen - ctx->offset, 0);
        if (n <= 0)
        {
            // send 出错
            if (n < 0)
            {
                ERROR("send");
            }
            ev_io_stop(w);
            close(w->fd);
            free(ctx->msg);
            free(ctx);
            return;
        }
        if (ctx->offset + n == ctx->msglen)
        {
            // 发送 DNS 消息完毕
            ev_io_stop(w);
            close(w->fd);
            free(ctx->msg);
            free(ctx);
        }
    }
}
