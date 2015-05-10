/*
 * db.c
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

#include <stdint.h>
#include <string.h>
#include "db.h"
#include "log.h"

#define HASH_SIZE 1031

static struct
{
	const char *name;
	int value;
} htab[HASH_SIZE];

static int hash1(const char *name)
{
	int h = 0;
	while (*name != '\0')
	{
		h = (h * 257 + (int)(uint8_t)(*name)) % HASH_SIZE;
		name++;
	}
	return h;
}

static int hash2(const char *name)
{
	int h = 0;
	while (*name != '\0')
	{
		h = (h * 257 ^ (int)(uint8_t)(*name)) % HASH_SIZE;
		name++;
	}
	return h;
}

int db_insert(const char *name, int value)
{
	int h1 = hash1(name);
	int h2 = hash2(name);

	for (int i = 0; i < HASH_SIZE; i++)
	{
		int h = (h1 + i * h2) % HASH_SIZE;
		if (htab[h].value == 0)
		{
			htab[h].name = name;
			htab[h].value = value;
			return 0;
		}
		else if (strcmp(htab[h].name, name) == 0)
		{
			return -1;
		}
	}
	LOG("warning: db full");
	return -1;
}

int db_search(const char *name)
{
	int h1 = hash1(name);
	int h2 = hash2(name);

	for (int i = 0; i < HASH_SIZE; i++)
	{
		int h = (h1 + i * h2) % HASH_SIZE;
		if (htab[h].value == 0)
		{
			return 0;
		}
		else if (strcmp(htab[h].name, name) == 0)
		{
			return htab[h].value;
		}
	}
	return 0;
}
