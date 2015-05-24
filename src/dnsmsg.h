/*
 * dnsmsg.h - DNS message I/O
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

#ifndef DNSMSG_H
#define DNSMSG_H

#include <stdint.h>
#include "query.h"


/*
 * @func query_recv()
 * @desc receive DNS query asynchronously
 * @memo @memo the query received will be inserted into query list
 */
extern void query_recv(int sock, int protocol, void (*cb)(uint16_t id));


/*
 * @func reply_recv()
 * @desc receive DNS reply asynchronously
 * @memo msg will be freed after callback
 *       if protocol is ns_tcp, sock will be closed after receive 
 */
extern void reply_recv(int sock, int protocol, void (*cb)(void *msg, int msglen));


/*
 * @func query_send()
 * @desc send DNS query
 * @memo synchronously for UDP, asynchronously for TCP
 */
extern void query_send(int sock, int protocol, void *msg, int msglen,
                       const struct sockaddr *addr, socklen_t addrlen);


/*
 * @func reply_send()
 * @desc send DNS reply
 * @memo synchronously for UDP, asynchronously for TCP
 *       if prot is TCP, sock will be closed after reply sent
 */
extern void reply_send(int sock, int protocol, void *msg, int msglen,
                       const struct sockaddr *addr, socklen_t addrlen);


#endif // DNSMSG_H
