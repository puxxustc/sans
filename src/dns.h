/*
 * dns.h
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

#ifndef DNS_H
#define DNS_H

#include <stdint.h>

#ifndef __MINGW32__
#  include <arpa/inet.h>
#endif


/*
* @desc DNS packet size
*/
#define NS_PACKETSZ 2048


/*
* @desc DNS name buffer size
*/
#define NS_NAMESZ 2048


/*
 * @type ns_flag
 * @desc DNS flags
 */
typedef struct
{
#if BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == PDP_ENDIAN || defined(__MINGW32__)
    // 第 1 个字节
    unsigned    rd :1;      // recursion desired
    unsigned    tc :1;      // truncated message
    unsigned    aa :1;      // authoritive answer
    unsigned    opcode :4;  // purpose of message
    unsigned    qr :1;      // response flag
    // 第 2 个字节
    unsigned    rcode :4;   // response code
    unsigned    cd: 1;      // checking disabled by resolver
    unsigned    ad: 1;      // authentic data from named
    unsigned    unused :1;  // unused bits
    unsigned    ra :1;      // recursion available
#else
    // 第 1 个字节
    unsigned    qr: 1;      // response flag
    unsigned    opcode: 4;  // purpose of message
    unsigned    aa: 1;      // authoritive answer
    unsigned    tc: 1;      // truncated message
    unsigned    rd: 1;      // recursion desired
    // 第 2 个字节
    unsigned    ra: 1;      // recursion available
    unsigned    unused :1;  // unused bits
    unsigned    ad: 1;      // authentic data from named
    unsigned    cd: 1;      // checking disabled by resolver
    unsigned    rcode :4;   // response code
#endif
} ns_flag;


/*
 * @type ns_header
 * @desc DNS message header
 */
typedef struct
{
    uint16_t id;        // id
    uint16_t flag;      // flag
    uint16_t qdcount;   // number of question entries
    uint16_t ancount;   // number of answer entries
    uint16_t nscount;   // number of authority entries
    uint16_t arcount;   // number of additional entries
} ns_header;


/*
 * @type ns_opcode
 * @desc DNS opcode
 */
typedef enum
{
    ns_o_query = 0,     // Standard query
    ns_o_iquery = 1,    // Inverse query
    ns_o_status = 2,    // Name server status query (unsupported)
    ns_o_notify = 4,    // Zone change notification
    ns_o_update = 5,    // Zone update message
    ns_o_max = 6
} ns_opcode;


/*
 * @type ns_rcode
 * @desc DNS response codes
 */
typedef enum
{
    ns_r_noerror = 0,   // No error occurred
    ns_r_formerr = 1,   // Format error
    ns_r_servfail = 2,  // Server failure
    ns_r_nxdomain = 3,  // Name error
    ns_r_notimpl = 4,   // Unimplemented
    ns_r_refused = 5,   // Operation refused
} ns_rcode;


/*
 * @type ns_class
 * @desc class
 */
typedef enum
{
    ns_c_in = 1,        // Internet
    ns_c_any = 255      // Wildcard match
} ns_class;


/*
 * @type ns_type
 * @desc values for resources and queries
 */
typedef enum
{
    ns_t_invalid = 0,
    ns_t_a = 1,         // Host address
    ns_t_ns = 2,        // Authoritative server
    ns_t_cname = 5,     // Canonical name
    ns_t_soa = 6,       // Start of authority zone
    ns_t_ptr = 12,      // Domain name pointer
    ns_t_mx = 15,       // Mail routing information
    ns_t_txt = 16,      // Text strings
    ns_t_aaaa = 28,     // Ip6 Address
    ns_t_any = 255,     // Wildcard match
    ns_t_block = 256, // Custom type, is blocked
} ns_type;


/*
 * @type ns_prot
 * @desc protocol
 */
typedef enum
{
    ns_udp = 1,
    ns_tcp = 2
} ns_prot;


/*
 * @func ns_getid()
 * @desc get id of DNS message
 */
extern uint16_t ns_getid(void *msg);


/*
 * @func ns_setid()
 * @desc set id of DNS message
 */
extern void ns_setid(void *msg, uint16_t id);


/*
 * @func ns_newid()
 * @desc generate a new unique ID
 */
extern uint16_t ns_newid(void);


/*
 * @func  ns_type_str()
 * @desc  convert DNS type to string
 * @param type - query type
 */
const char *ns_type_str(int type);


/*
 * @func  ns_mkquery()
 * @desc  make DNS query
 * @param buf    - buffer
 *        buflen - length of buffer
 *        name   - domain name
 *        type   - query type
 */
extern int ns_mkquery(void *buf, int buflen, const char *name, int type);


/*
 * @func  ns_parse_query()
 * @desc  parse DNS query
 * @param msg    - message
 *        msglen - length of message
 *        name   - domain name
 *        type   - query type
 */
extern int ns_parse_query(void *msg, int msglen, char *dname, int *type);


extern int ns_parse_reply(void *msg, int msglen, char *dname, int *type);


#endif // DNS_H
