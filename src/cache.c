/*
 * cache.c - DNS cache
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
#include <string.h>
#include "cache.h"
#include "dns.h"
#include "log.h"


/*
 * @type entry_t
 * @desc hash table entry
 */
typedef struct entry_t
{
    cache_t *data;
    struct entry_t *next;
} entry_t;


/*
 * @var  htable
 * @desc hash table to store DNS records
 */
#define HASH_SIZE 2039
static entry_t * htable[HASH_SIZE];


/*
 * @func hash()
 * @desc hash function
 */
static int hash(const char *name, int type)
{
    int h = type;
    while (*name != '\0')
    {
        h = (h * 257 + (int)(uint8_t)(*name)) % HASH_SIZE;
        name++;
    }
    return h;
}


/*
 * @func  cache_insert()
 * @desc  insert an item into hash table
 * @param name  - domain name
 *        cache - cache item
 */
int cache_insert(cache_t *cache)
{
    int h = hash(cache->name, cache->type);

    entry_t *entry = htable[h];

    while (entry != NULL)
    {
        if ((entry->data->type == cache->type)
            && (strcmp(entry->data->name, cache->name) == 0))
        {
            // 要插入的条目已经存在
            return -1;
        }
        entry = entry->next;
    }

    entry = (entry_t *)malloc(sizeof(entry_t));
    if (entry == NULL)
    {
        LOG("out of memory");
        return -1;
    }
    entry->data = cache;
    entry->next = htable[h];
    htable[h] = entry;
    return 0;
}


/*
 * @func  cache_search()
 * @desc  search in cache
 * @param name - domain name
 *        type - record type
 * @ret   pointer to cache item, or NULL
 */
cache_t *cache_search(const char *name, int type)
{
    int h = hash(name, type);

    entry_t *entry = htable[h];

    while (entry != NULL)
    {
        if ((entry->data->type == type) && (strcmp(entry->data->name, name) == 0))
        {
            return entry->data;
        }
        entry = entry->next;
    }
    return NULL;
}


/*
 * @func  cache_delete()
 * @desc  delete cache item
 * @param name - domain name
 *        type - record type
 */
int cache_delete(const char *name, int type)
{
    int h = hash(name, type);

    entry_t *entry = htable[h];
    entry_t *last = NULL;

    while (entry != NULL)
    {
        if ((entry->data->type == type) && (strcmp(entry->data->name, name) == 0))
        {
            if (last == NULL)
            {
                htable[h] = NULL;
                free(entry);
            }
            else
            {
                last->next = entry->next;
                free(entry);
            }
            return 0;
        }
        last = entry;
        entry = entry->next;
    }
    return -1;
}


/*
 * @func cache_tick(void)
 * @desc tick every seconds
 */
void cache_tick(void)
{
    for (int i = 0; i < HASH_SIZE; i++)
    {
        entry_t *entry = htable[i];
        entry_t *last = NULL;
        while (entry != NULL)
        {
            entry->data->ttl--;
            if (entry->data->ttl == 0)
            {
                if (last == NULL)
                {
                    htable[i] = NULL;
                    free(entry);
                }
                else
                {
                    last->next = entry->next;
                    free(entry);
                }
            }
            last = entry;
            entry = entry->next;
        }
    }
}
