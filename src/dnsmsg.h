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

extern void query_recv(int sock, proto_t proto, void (*cb)(uint16_t id));
extern void reply_recv(int sock, proto_t proto, void (*cb)(void *msg, int msglen));
extern void query_send(int sock, proto_t proto, void *msg, int msglen,
                       const struct sockaddr *addr, socklen_t addrlen);
extern void reply_send(int sock, proto_t proto, void *msg, int msglen,
                       const struct sockaddr *addr, socklen_t addrlen);

#endif // DNSMSG_H
