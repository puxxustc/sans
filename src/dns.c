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
#include <stdlib.h>
#include <string.h>

#ifdef __MINGW32__
#  include "win.h"
#else
#  include <netinet/in.h>
#endif

#include "dns.h"
#include "query.h"
#include "resolv.h"
#include "utils.h"


/*
 * @func ns_getid()
 * @desc get ID of DNS message
 */
uint16_t ns_getid(void *msg)
{
    return ntohs(((ns_header *)msg)->id);
}


/*
 * @func ns_setid()
 * @desc set ID of DNS message
 */
void ns_setid(void *msg, uint16_t id)
{
    ((ns_header *)msg)->id = htons(id);
}


/*
 * @func ns_newid()
 * @desc generate a new unique ID
 */
uint16_t ns_newid(void)
{
    uint16_t id;
    do
    {
        id = rand_uint16();
    } while ((id == 0) || (query_search(id) != NULL));
    return id;
}


/*
 * @func  ns_type_str()
 * @desc  convert DNS type to string
 * @param type - query type
 */
const char *ns_type_str(int type)
{
    const char *str = "UNKNOWN";
    switch (type)
    {
    case ns_t_a:
        str = "A";
        break;
    case ns_t_ns:
        str = "NS";
        break;
    case ns_t_cname:
        str = "CNAME";
        break;
    case ns_t_soa:
        str = "SOA";
        break;
    case ns_t_ptr:
        str = "PTR";
        break;
    case ns_t_mx:
        str = "MX";
        break;
    case ns_t_txt:
        str = "TXT";
        break;
    case ns_t_aaaa:
        str = "AAAA";
        break;
    case ns_t_any:
        str = "ANY";
        break;
    default:
        break;
    }
    return str;
}


/*
 * @func  ns_mkquery()
 * @desc  make DNS query
 * @param buf    - buffer
 *        buflen - length of buffer
 *        name   - domain name
 *        tpye   - query type
 */
int ns_mkquery(void *buf, int buflen, const char *name, int type)
{
    assert(buf != NULL);
    assert(buflen > NS_HFIXEDSZ);

    ns_header *hp;
    void *cp;
    int n;
    u_char *dnptrs[20], **dpp, **lastdnptr;

    // 初始化 DNS 头
    bzero(buf, NS_HFIXEDSZ);
    hp = (ns_header *)buf;

    hp->id = rand_uint16();
    ns_flag flag =
    {
        .opcode = ns_o_query,
        .rd = 1,
        .rcode = ns_r_noerror
    };
    memcpy(&(hp->flag), &flag, 2);
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


/*
 * @func  ns_parse_query()
 * @desc  parse DNS query
 * @param msg    - message
 *        msglen - length of message
 *        name   - domain name
 *        type   - query type
 */
int ns_parse_query(void *msg, int msglen, char *name, int *type)
{
    ns_msg nsmsg;
    ns_rr nsrr;

    assert(msg != NULL);
    assert(msglen > 0);
    assert(name != NULL);
    assert(type != NULL);

    if (ns_initparse((const u_char *)msg, msglen, &nsmsg) < 0)
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
    strcpy(name, ns_rr_name(nsrr));
    *type = (int)ns_rr_type(nsrr);

    return 0;
}


/*
 * @func  ns_parse_reply()
 * @desc  parse DNS reply
 * @param msg    - message
 *        msglen - length of message
 *        name   - domain name
 *        type   - query type
 */
int ns_parse_reply(void *msg, int msglen, char *name, int *type)
{
    ns_msg nsmsg;
    ns_rr nsrr;

    assert(msg != NULL);
    assert(msglen > 0);
    assert(name != NULL);
    assert(type != NULL);

    if (ns_initparse((const u_char *)msg, msglen, &nsmsg) < 0)
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
    strcpy(name, ns_rr_name(nsrr));
    if (ns_msg_count(nsmsg, ns_s_an) == 0)
    {
        *type = ns_t_invalid;
    }
    else
    {
        if (ns_parserr(&nsmsg, ns_s_an, 0, &nsrr) != 0)
        {
            return -1;
        }
        *type = (int)ns_rr_type(nsrr);
    }

    return 0;
}
