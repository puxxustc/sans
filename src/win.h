/*
 * win.c - Win32 port helpers
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

#ifndef WIN_H
#define WIN_H

#ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#ifndef EINPROGRESS
#  define EINPROGRESS WSAEWOULDBLOCK
#endif

#ifndef MSG_NOSIGNAL
#  define MSG_NOSIGNAL 0
#endif

#ifdef errno
#  undef errno
#endif
#define errno WSAGetLastError()

#define recv(a, b, c, d) recv(a, (char *)b, c, d)
#define send(a, b, c, d) send(a, (char *)b, c, d)
#define recvfrom(a, b, c, d, e, f) recvfrom(a, (char *)b, c, d, e, f)
#define sendto(a, b, c, d, e, f) sendto(a, (char *)b, c, d, e, f)
#define getsockopt(a, b, c, d, e) getsockopt(a, b, c, (char *)(d), e)
#define setsockopt(a, b, c, d, e) setsockopt(a, b, c, (char *)(d), e)
#define bzero(a, b) memset(a, 0, b)

#endif
