/*
 * log.c - print log message
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

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef __MINGW32__
#  include "win.h"
#endif

#include "log.h"


/*
 * @func sans_log()
 * @desc pring log info with timestamp
 */
void sans_log(FILE *stream, const char *format, ...)
{
    time_t now = time(NULL);
    char timestr[20];
    strftime(timestr, 20, "%y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(stream, "[%s] ", timestr);

    va_list args;
    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
    putchar('\n');
    fflush(stream);
}


/*
 * @func sans_err()
 * @desc pring error message with timestamp
 */
void sans_err(const char *msg)
{
#ifdef __MINGW32__
    LPVOID *s = NULL;
    FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, WSAGetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&s, 0, NULL);
    if (s != NULL)
    {
        sans_log(stderr, "%s: %s", msg, s);
        LocalFree(s);
    }
#else
    sans_log(stderr, "%s: %s", msg, strerror(errno));
#endif
}


/*
 * @func sans_dump()
 * @desc dump binary data
 */
#ifndef DEBUG
void sans_dump(void *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%02x ", *(uint8_t *)(buf + i));
        if (i % 16 == 15)
        {
            putchar('\n');
        }
    }
    if (len % 16 != 0)
    {
        putchar('\n');
    }
    fflush(stdout);
}
#endif
