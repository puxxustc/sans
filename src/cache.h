/*
 * cache.h - DNS cache
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

#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include "dns.h"

/*
 * @type ns_a
 * @desc A record
 */
typedef uint32_t ns_a;


/*
 * @type ns_aaaa
 * @desc AAAA record
 */
typedef uint8_t ns_aaaa[16];


/*
 * @type ns_ns
 * @desc NS record
 */
typedef char ns_ns[NS_NAMESZ];


/*
 * @type ns_cname
 * @desc CNAME record
 */
typedef char ns_cname[NS_NAMESZ];


/*
 * @type ns_mx
 * @desc MX record
 */
typedef struct ns_mx
{
    int priority;
    char mx[NS_NAMESZ];
} ns_mx;


/*
 * @type ns_soa
 * @desc SOA record
 */
typedef struct ns_soa
{
    char mname[NS_NAMESZ];
    char rname[NS_NAMESZ];
    uint32_t serial;
    uint32_t refresh;
    uint32_t retry;
    uint32_t expire;
    uint32_t minimum;
} ns_soa;


/*
 * @type ns_txt
 * @desc TXT record
 */
typedef char ns_txt[NS_NAMESZ];


/*
 * @type ns_ptr
 * @desc PTR record
 */
typedef char ns_ptr[NS_NAMESZ];


/*
 * @type ns_block
 * @desc custom record type, 1 means this domain is blocked, 0 means not
 */
typedef int ns_block;


/*
 * @type cache_t
 * @desc cache item
 */
typedef struct cache_t
{
    char name[NS_NAMESZ];   // domain name
    uint32_t ttl;           // TTL
    int type;               // record type
    int count;              // record count
    uint8_t data[0];        // data
} cache_t;


/*
 * @func  cache_insert()
 * @desc  insert an item into hash table
 * @param cache - cache item
 */
extern int cache_insert(cache_t *cache);


/*
 * @func  cache_search()
 * @desc  search in cache
 * @param name - domain name
 *        type - record type
 * @ret   pointer to cache item
 */
extern cache_t *cache_search(const char *name, int type);


/*
 * @func cache_tick(void)
 * @desc tick every seconds
 */
extern void cache_tick(void);


#endif // CACHE_H
