/*
 * dname.c - domain name
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dname.h"
#include "log.h"

#define HASH_SIZE 1031
#define MAGIC 0x616e6f72

typedef struct dname_t
{
	int magic;
	int use;
	struct dname_t *next;
	char name[];
} dname_t;

#define OVERHEAD (offsetof(dname_t, name))

dname_t **htab;

static int hash(const char *name)
{
	int h = 0;
	while (*name != '\0')
	{
		h = (h * 257 + (int)(uint8_t)(*name)) % HASH_SIZE;
		name++;
	}
	return h;
}

int dname_init(void)
{
	htab = (dname_t **)malloc(sizeof(dname_t *) * HASH_SIZE);
	if (htab == NULL)
	{
		LOG("out of memory");
		return -1;
	}
	bzero(htab, sizeof(dname_t *) * HASH_SIZE);
	return 0;
}

static dname_t *hash_insert(const char *name)
{
	int len = strlen(name);
	dname_t *dname = (dname_t *)malloc(OVERHEAD + len + 1);
	if (dname == NULL)
	{
		LOG("out of memory");
		return NULL;
	}
	dname->magic = MAGIC;
	dname->use = 1;
	memcpy(dname->name, name, len + 1);
	int h = hash(dname->name);
	dname->next = htab[h];
	htab[h] = dname;
	return dname;
}

static dname_t *hash_search(const char *name)
{
	dname_t *p = htab[hash(name)];
	while (p != NULL)
	{
		if (strcmp(p->name, name) == 0)
		{
			return p;
		}
		p = p->next;
	}
	return NULL;
}

static int hash_del(const char *name)
{
	int h = hash(name);
	dname_t *p = htab[h];
	dname_t *last = NULL;
	while (p != NULL)
	{
		if (strcmp(p->name, name) == 0)
		{
			if (last == NULL)
			{
				htab[h] = NULL;
			}
			else
			{
				last->next = p->next;
			}
			free(p);
			return 0;
		}
		last = p;
		p = p->next;
	}
	return -1;
}

char *dname_new(const char *name)
{
	dname_t *dname = hash_search(name);
	if (dname != NULL)
	{
		dname->use++;
		return dname->name;
	}

	dname = hash_insert(name);
	if (dname != NULL)
	{
		return dname->name;
	}
	else
	{
		return NULL;
	}
}

char *dname_dup(const char *name)
{
	dname_t *dname = (dname_t *)(name - OVERHEAD);

	assert(dname->magic == MAGIC);
	assert(dname->use >= 1);

	dname->use++;
	return dname->name;
}

void dname_free(char *name)
{
	dname_t *dname = (dname_t *)(name - OVERHEAD);

	assert(dname->magic == MAGIC);
	assert(dname->use >= 1);

	dname->use--;
	if (dname->use == 0)
	{
		hash_del(name);
	}
}
