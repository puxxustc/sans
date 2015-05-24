/*
 * event.c - simple event loop
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

#include <assert.h>
#include <stddef.h>
#include <sys/select.h>
#include <sys/time.h>
#include "event.h"


/*
 * @var  run
 * @desc should run
 */
static volatile int run;


/*
* @var  twcb
* @desc timeout callback
*/
static void (*twcb)(void);


/*
 * @var  wlist
 * @desc event watcher list
 */
#define WLIST_SIZE 128
static ev_io *wlist[WLIST_SIZE];


/*
 * @var  tv
 * @desc timestamp
 */
static struct timeval tv;


/*
 * @func  ev_init()
 * @desc  initialize event loop
 * @param cb - timeout callback
 */
int ev_init(void (*cb)(void))
{
    run = 1;
    twcb = cb;
    gettimeofday(&tv, NULL);
    return 0;
}


/*
 * @func  ev_io_init()
 * @desc  initialize io watcher
 * @param w     - watcher
 *        cb    - callback
 *        fd    - file descriptor
 *        event - event wait for
 */
void ev_io_init(ev_io *w, void (*cb)(struct ev_io *w), int fd, int event)
{
    assert(fd > 0);
    assert(cb != NULL);
    assert((event == EV_READ) || (event == EV_WRITE));

    w->fd = fd;
    w->event = event;
    w->cb = cb;
}


/*
 * @func  ev_io_start()
 * @desc  start io watcher
 * @param w - watcher
 */
void ev_io_start(ev_io *w)
{
    for (int i = 0; i < WLIST_SIZE; i++)
    {
        if (wlist[i] == NULL)
        {
            wlist[i] = w;
            return;
        }
    }
    assert("wlist full" == NULL);
}


/*
 * @func  ev_io_stop()
 * @desc  stop io watcher
 * @param w - watcher
 */
void ev_io_stop(ev_io *w)
{
    for (int i = 0; i < WLIST_SIZE; i++)
    {
        if (wlist[i] == w)
        {
            wlist[i] = NULL;
            return;
        }
    }
    assert("bad watcher" == NULL);
}


/*
 * @func ev_poll()
 * @desc wait for events
 * @ret  event count
 */
static int ev_poll(void)
{
    fd_set rfds, wfds;
    int max_fd = -1;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    for (int i = 0; i < WLIST_SIZE; i++)
    {
        if (wlist[i] != NULL)
        {
            if (wlist[i]->fd > max_fd)
            {
                max_fd = wlist[i]->fd;
            }
            if (wlist[i]->event == EV_READ)
            {
                FD_SET(wlist[i]->fd, &rfds);
            }
            else if (wlist[i]->event == EV_WRITE)
            {
                FD_SET(wlist[i]->fd, &wfds);
            }
        }
    }
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    int r = select(max_fd + 1, &rfds, &wfds, NULL, &timeout);
    if (r > 0)
    {
        for (int i = 0; i < WLIST_SIZE; i++)
        {
            if ((wlist[i] != NULL) && (wlist[i]->event == EV_READ) && FD_ISSET(wlist[i]->fd, &rfds))
            {
                (wlist[i]->cb)(wlist[i]);
            }
            if ((wlist[i] != NULL) && (wlist[i]->event == EV_WRITE) && FD_ISSET(wlist[i]->fd, &wfds))
            {
                (wlist[i]->cb)(wlist[i]);
            }
        }
    }
    return r;
}


/*
 * @func ev_run()
 * @desc start event loop
 */
void ev_run(void)
{
    while (run)
    {
        ev_poll();
        struct timeval t;
        gettimeofday(&t, NULL);
        if ((1000000 * (t.tv_sec - tv.tv_sec) + t.tv_usec - tv.tv_usec) > 1000000)
        {
            tv = t;
            (twcb)();
        }
    }
}


/*
 * @func ev_stop()
 * @desc stop event loop
 */
void ev_stop(void)
{
    run = 0;
}
