/*
 * dns.c
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
#include <string.h>

#ifdef __MINGW32__
#  include "win.h"
#else
#  include <netinet/in.h>
#endif

#include "dname.h"
#include "dns.h"
#include "resolv.h"
#include "utils.h"

uint16_t ns_getid(void *msg)
{
	return ntohs(((ns_header *)msg)->id);
}

void ns_setid(void *msg, uint16_t id)
{
	((ns_header *)msg)->id = htons(id);
}

int ns_mkquery(void *buf, int buflen, const char *name, int type)
{
	ns_header *hp;
	void *cp;
	int n;
	u_char *dnptrs[20], **dpp, **lastdnptr;

	// 初始化 DNS 头
	if ((buf == NULL) || (buflen < NS_HFIXEDSZ))
	{
		return -1;
	}
	bzero(buf, NS_HFIXEDSZ);
	hp = (ns_header *)buf;

	hp->id = rand_uint16();
	hp->flag.opcode = ns_o_query;
	hp->flag.rd = 1;
	hp->flag.rcode = ns_r_noerror;
	cp = buf + NS_HFIXEDSZ;
	buflen -= NS_HFIXEDSZ;
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof dnptrs / sizeof dnptrs[0];

	if ((buflen -= NS_QFIXEDSZ) < 0)
	{
		return -1;
	}
	n = ns_name_compress(name, cp, buflen,
	                     (const u_char **)dnptrs,
	                     (const u_char **)lastdnptr);
	if (n < 0)
	{
		return -1;
	}
	cp += n;
	buflen -= n;
	NS_PUT16(type, cp);
	NS_PUT16(ns_c_in, cp);
	hp->qdcount = htons(1);
	return (void *)cp - buf;
}

int ns_parse_query(void *msg, int len, char **dname, int *qtype)
{
	ns_msg nsmsg;
	ns_rr nsrr;

	assert(msg != NULL);
	assert(len > 0);
	assert(dname != NULL);
	assert(qtype != NULL);

	if (ns_initparse((const u_char *)msg, len, &nsmsg) < 0)
	{
		return -1;
	}
	if (ns_msg_count(nsmsg, ns_s_qd) == 0)
	{
		return -1;
	}
	if (ns_parserr(&nsmsg, ns_s_qd, 0, &nsrr) != 0)
	{
		return -1;
	}
	*dname = dname_new(ns_rr_name(nsrr));
	if (*dname == NULL)
	{
		return -1;
	}
	*qtype = (int)ns_rr_type(nsrr);

	return 0;
}

int ns_parse_reply(void *msg, int len, char **dname, int *qtype)
{
	ns_msg nsmsg;
	ns_rr nsrr;

	assert(msg != NULL);
	assert(len > 0);
	assert(dname != NULL);
	assert(qtype != NULL);

	if (ns_initparse((const u_char *)msg, len, &nsmsg) < 0)
	{
		return -1;
	}
	if (ns_msg_count(nsmsg, ns_s_qd) == 0)
	{
		return -1;
	}
	if (ns_parserr(&nsmsg, ns_s_qd, 0, &nsrr) != 0)
	{
		return -1;
	}
	*dname = dname_new(ns_rr_name(nsrr));
	if (*dname == NULL)
	{
		return -1;
	}
	if (ns_msg_count(nsmsg, ns_s_an) == 0)
	{
		*qtype = ns_t_invalid;
	}
	else
	{
		if (ns_parserr(&nsmsg, ns_s_an, 0, &nsrr) != 0)
		{
			dname_free(*dname);
			return -1;
		}
		*qtype = (int)ns_rr_type(nsrr);
	}

	return 0;
}
