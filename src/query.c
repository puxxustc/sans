/*
 * query.c - Manage DNS queries
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

#define QUERY_MAX 128

static query_t *(querys[QUERY_MAX]);

uint16_t query_newid(query_t *query)
{
	uint16_t id;
	do
	{
		id = rand_uint16();
	} while ((id == 0) || (query_search(id) != NULL));
	query->id = id;
	ns_setid(query->msg, id);
	return id;
}

int query_add(query_t *query)
{
	for (int i = 0; i < QUERY_MAX; i++)
	{
		if (querys[i] == NULL)
		{
			query->ori_id = ns_getid(query->msg);
			query->id = query->ori_id;
			querys[i] = query;
			return 0;
		}
	}
	return -1;
}

query_t *query_search(uint16_t id)
{
	for (int i = 0; i < QUERY_MAX; i++)
	{
		if ((querys[i] != NULL) && (querys[i]->id == id))
		{
			return querys[i];
		}
	}
	return NULL;
}

void query_del(uint16_t id)
{
	for (int i = 0; i < QUERY_MAX; i++)
	{
		if ((querys[i] != NULL) && (querys[i]->id == id))
		{
			free(querys[i]);
			querys[i] = NULL;
		}
	}
}
