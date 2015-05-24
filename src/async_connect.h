/*
 * async_connect.h - async connect
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

#ifndef ASYNC_CONNECT_H
#define ASYNC_CONNECT_H

#ifdef __MINGW32__
#  include "win.h"
#else
#  include <sys/socket.h>
#endif


/*
 * @func  socks5_init()
 * @desc  initialize SOCKS5 server address
 * @param host - host of IP address of SOCKS5 server
 *        port - port of SOCKS5 server
 */
extern int socks5_init(const char *host, const char *port);


/*
 * @func  async_connect()
 * @desc  async connect (support SOCKS5)
 * @param addr    - peer address
 *        addrlen - length of addr
 *        cb      - callback
 *        socks5  - connect via SOCK5 or not
 *        data    - additional data
 */
extern void async_connect(const struct sockaddr *addr, socklen_t addrlen,
                          void (*cb)(int, void *), int socks5, void *data);


#endif // ASYNC_CONNECT_H
