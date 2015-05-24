/*
 * query.h - manage DNS queries
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


#include "dns.h"


/*
 * @type query_t
 * @desc DNS query
 */
typedef struct
{
    uint16_t id;
    uint16_t qid;
    int ttl;
    int sock;
    int protocol;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    int type;
    char name[NS_NAMESZ];
} query_t;


/*
 * @func query_add()
 * @desc add new DNS query
 */
extern int query_add(query_t *query);


/*
 * @func  query_search()
 * @desc  search DNS query from query list
 * @param id - query id
 */
extern query_t *query_search(uint16_t id);


/*
 * @func  query_delete()
 * @desc  delete DNS query from query list
 * @param id - query id
 */
extern int query_delete(uint16_t id);


/*
 * @func query_tick()
 * @desc delete old queries periodically
 */
extern void query_tick(void);


#endif // QUERY_H
