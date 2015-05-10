/*
 * query.h - Manage DNS queries
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

#ifndef QUERY_H
#define QUERY_H

#include <stdint.h>

#ifdef __MINGW32__
#  include "win.h"
#else
#  include <sys/socket.h>
#endif

#define BUF_SIZE 2048

typedef enum
{
	udp = 1,
	tcp = 2
} proto_t;

typedef struct
{
	uint16_t id;
	uint16_t ori_id;
	int sock;
	proto_t proto;
	int msglen;
	int state;
	socklen_t addrlen;
	struct sockaddr_storage addr;
	char msg[BUF_SIZE];
} query_t;

extern uint16_t query_newid(query_t *query);
extern int query_add(query_t *query);
extern query_t *query_search(uint16_t id);
extern void query_del(uint16_t id);

#endif // QUERY_H
