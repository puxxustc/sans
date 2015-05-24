/*
 * log.h - print log message
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

#ifndef LOG_H
#define LOG_H

#include <stdio.h>


/*
 * @func sans_log()
 * @desc pring log info with timestamp
 */
extern void sans_log(FILE *stream, const char *format, ...);


/*
 * @func sans_err()
 * @desc pring error message with timestamp
 */
extern void sans_err(const char *msg);


/*
 * @func sans_dump()
 * @desc dump binary data
 */
#ifndef DEBUG
extern void sans_dump(void *buf, size_t len);
#endif

#define LOG(format, ...)  do{sans_log(stdout, format, ##__VA_ARGS__);}while(0)


#ifdef ERROR
#  undef ERROR
#endif
#define ERROR(msg)  do{sans_err(msg);}while(0)

#ifndef DEBUG
#  define DUMP sans_dump
#else
#  define DUMP do{}while(0)
#endif


#endif // LOG_H
