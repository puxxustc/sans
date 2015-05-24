/*
 * sans.h - simple anti-poultion name server
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

#ifndef SANS_H
#define SANS_H

#include "conf.h"


/*
 * @func sans_init()
 * @desc initialize
 */
extern int sans_init(const conf_t *conf);


/*
 * @func sans_run()
 */
extern int sans_run(void);


/*
 * @func sans_stop()
 */
extern void sans_stop(void);


#endif // SANS_H
