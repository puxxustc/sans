/*
 * utils.h - some util functions
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

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>


/*
 * @func rand_uint16()
 * @desc computes a pseudo-random uint16
 */
extern uint16_t rand_uint16(void);


/*
 * @func setnonblock()
 * @desc set fd in nonblock mode
 */
extern int setnonblock(int fd);


/*
 * @func settimeout()
 * @desc set timeout of socket
 */
extern int settimeout(int fd);


/*
 * @func setreuseaddr()
 * @desc set SO_REUSEADDR of socket
 */
extern int setreuseaddr(int fd);


/*
 * @func setkeepalive()
 * @desc set SO_KEEPALIVE of socket
 */
extern int setkeepalive(int fd);


/*
 * @func setnosigpipe()
 * @desc set SO_NOSIGPIPE of socket
 */
#ifdef SO_NOSIGPIPE
extern int setnosigpipe(int fd);
#endif


/*
 * @func getsockerror()
 * @desc get socket error
 */
extern int getsockerror(int fd);


/*
 * @func runas()
 * @desc run as user
 */
extern int runas(const char *user);


/*
 * @func daemonize()
 * @desc daemonize to background
 */
extern int daemonize(const char *pidfile, const char *logfile);


#endif // UTILS_H
