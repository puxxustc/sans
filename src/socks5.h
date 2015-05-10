/*
 * socks5.h - SOCKS5 Protocol
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

#ifndef SOCKS5_H
#define SOCKS5_H

#ifdef __MINGW32__
#  include "win.h"
#else
#  include <sys/socket.h>
#endif

extern int socks5_init(const char *host, const char *port);
extern void socks5_connect(const struct sockaddr *addr, socklen_t addrlen,
                           void (*cb)(int, void *), void *data);

#endif
