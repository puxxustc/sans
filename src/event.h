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

#ifndef EVENT_H
#define EVENT_H


/*
 * @const
 * @desc  events
 */
enum
{
    EV_NONE   =  0x00,  // no events
    EV_READ   =  0x01,  // read will not block
    EV_WRITE  =  0x02,  // write will not block
};


/*
 * @type ev_io
 * @desc io watcher
 */
typedef struct ev_io
{
    int fd;
    int event;
    void (*cb)(struct ev_io *w);
    void *data;
} ev_io;


/*
 * @func  ev_init()
 * @desc  initialize event loop
 * @param cb - timeout callback
 * @ret   0 - if succeed
 *        !0 - if failed
 */
extern int ev_init(void (*cb)(void));


/*
 * @func  ev_io_init()
 * @desc  initialize io watcher
 * @param w - watcher
 *        cb - callback
 *        fd - file descriptor
 *        event - event wait for
 */
extern void ev_io_init(ev_io *w, void (*cb)(struct ev_io *w), int fd, int event);


/*
 * @func  ev_io_start()
 * @desc  start io watcher
 * @param w - watcher
 */
extern void ev_io_start(ev_io *w);


/*
 * @func  ev_io_stop()
 * @desc  stop io watcher
 * @param w - watcher
 */
extern void ev_io_stop(ev_io *w);


/*
 * @func ev_run()
 * @desc start event loop
 */
extern void ev_run(void);


/*
 * @func ev_stop()
 * @desc stop event loop
 */
extern void ev_stop(void);


#endif
