/*
 * query.c - manage DNS queries
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

#include <stdlib.h>
#include "dns.h"
#include "query.h"
#include "utils.h"


/*
 * @var  qlist
 * @desc query list
 */
#define QLIST_SIZE 128
static query_t * qlist[QLIST_SIZE];


/*
 * @func query_add()
 * @desc add new DNS query
 */
int query_add(query_t *query)
{
    query->ttl = 6;
    query->qid = query->id;
    for (int i = 0; i < QLIST_SIZE; i++)
    {
        if (qlist[i] == NULL)
        {
            qlist[i] = query;
            return 0;
        }
    }
    return -1;
}


/*
 * @func  query_search()
 * @desc  search DNS query from query list
 * @param id - query id
 */
query_t *query_search(uint16_t id)
{
    for (int i = 0; i < QLIST_SIZE; i++)
    {
        if ((qlist[i] != NULL) && (qlist[i]->id == id))
        {
            return qlist[i];
        }
    }
    return NULL;
}


/*
 * @func  query_delete()
 * @desc  delete DNS query from query list
 * @param id - query id
 */
int query_delete(uint16_t id)
{
    for (int i = 0; i < QLIST_SIZE; i++)
    {
        if ((qlist[i] != NULL) && (qlist[i]->id == id))
        {
            free(qlist[i]);
            qlist[i] = NULL;
            return 0;
        }
    }
    return -1;
}


/*
 * @func query_tick()
 * @desc delete old queries periodically
 */
void query_tick(void)
{
    for (int i = 0; i < QLIST_SIZE; i++)
    {
        if (qlist[i] != NULL)
        {
            qlist[i]->ttl--;
            if (qlist[i]->ttl == 0)
            {
                free(qlist[i]);
                qlist[i] = NULL;
            }
        }
    }
}
