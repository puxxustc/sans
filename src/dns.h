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

// DNS 标志
typedef struct
{
#if BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == PDP_ENDIAN || defined(__MINGW32__)
	// 第 1 个字节
	unsigned	rd :1;		// recursion desired
	unsigned	tc :1;		// truncated message
	unsigned	aa :1;		// authoritive answer
	unsigned	opcode :4;	// purpose of message
	unsigned	qr :1;		// response flag
	// 第 2 个字节
	unsigned	rcode :4;	// response code
	unsigned	cd: 1;		// checking disabled by resolver
	unsigned	ad: 1;		// authentic data from named
	unsigned	unused :1;	// unused bits
	unsigned	ra :1;		// recursion available
#else
	// 第 1 个字节
	unsigned	qr: 1;		// response flag
	unsigned	opcode: 4;	// purpose of message
	unsigned	aa: 1;		// authoritive answer
	unsigned	tc: 1;		// truncated message
	unsigned	rd: 1;		// recursion desired
	// 第 2 个字节
	unsigned	ra: 1;		// recursion available
	unsigned	unused :1;	// unused bits
	unsigned	ad: 1;		// authentic data from named
	unsigned	cd: 1;		// checking disabled by resolver
	unsigned	rcode :4;	// response code
#endif
} ns_flag;

// DNS 消息头
typedef struct
{
	uint16_t id;		// query id
	ns_flag	flag;		// flag
	uint16_t qdcount;	// number of question entries
	uint16_t ancount;	// number of answer entries
	uint16_t nscount;	// number of authority entries
	uint16_t arcount;	// number of additional entries
} ns_header;

// opcode
typedef enum
{
	ns_o_query = 0,		// Standard query
	ns_o_iquery = 1,	// Inverse query
	ns_o_status = 2,	// Name server status query (unsupported)
	ns_o_notify = 4,	// Zone change notification
	ns_o_update = 5,	// Zone update message
	ns_o_max = 6
} ns_opcode;

// response codes
typedef	enum
{
	ns_r_noerror = 0,	// No error occurred
	ns_r_formerr = 1,	// Format error
	ns_r_servfail = 2,	// Server failure
	ns_r_nxdomain = 3,	// Name error
	ns_r_notimpl = 4,	// Unimplemented
	ns_r_refused = 5,	// Operation refused
} ns_rcode;

// values for class field
typedef enum
{
	ns_c_invalid = 0,	// Cookie
	ns_c_in = 1,		// Internet
	ns_c_2 = 2,			// unallocated/unsupported
	ns_c_chaos = 3,		// MIT Chaos-net
	ns_c_hs = 4,		// MIT Hesiod
	// Query class values which do not appear in resource records
	ns_c_none = 254,	// for prereq. sections in update requests
	ns_c_any = 255,		// Wildcard match
	ns_c_max = 65536
} ns_class;

// values for resources and queries.
typedef enum
{
	ns_t_invalid = 0,	// Cookie
	ns_t_a = 1,			// Host address
	ns_t_ns = 2,		// Authoritative server
	ns_t_md = 3,		// Mail destination
	ns_t_mf = 4,		// Mail forwarder
	ns_t_cname = 5,		// Canonical name
	ns_t_soa = 6,		// Start of authority zone
	ns_t_mb = 7,		// Mailbox domain name
	ns_t_mg = 8,		// Mail group member
	ns_t_mr = 9,		// Mail rename name
	ns_t_null = 10,		// Null resource record
	ns_t_wks = 11,		// Well known service
	ns_t_ptr = 12,		// Domain name pointer
	ns_t_hinfo = 13,	// Host information
	ns_t_minfo = 14,	// Mailbox information
	ns_t_mx = 15,		// Mail routing information
	ns_t_txt = 16,		// Text strings
	ns_t_rp = 17,		// Responsible person
	ns_t_afsdb = 18,	// AFS cell database
	ns_t_x25 = 19,		// X_25 calling address
	ns_t_isdn = 20,		// ISDN calling address
	ns_t_rt = 21,		// Router
	ns_t_nsap = 22,		// NSAP address
	ns_t_nsap_ptr = 23,	// Reverse NSAP lookup (deprecated)
	ns_t_sig = 24,		// Security signature
	ns_t_key = 25,		// Security key
	ns_t_px = 26,		// X.400 mail mapping
	ns_t_gpos = 27,		// Geographical position (withdrawn)
	ns_t_aaaa = 28,		// Ip6 Address
	ns_t_loc = 29,		// Location Information
	ns_t_nxt = 30,		// Next domain (security)
	ns_t_eid = 31,		// Endpoint identifier
	ns_t_nimloc = 32,	// Nimrod Locator
	ns_t_srv = 33,		// Server Selection
	ns_t_atma = 34,		// ATM Address
	ns_t_naptr = 35,	// Naming Authority PoinTeR
	ns_t_kx = 36,		// Key Exchange
	ns_t_cert = 37,		// Certification record
	ns_t_a6 = 38,		// IPv6 address (deprecated, use ns_t_aaaa)
	ns_t_dname = 39,	// Non-terminal DNAME (for IPv6)
	ns_t_sink = 40,		// Kitchen sink (experimentatl)
	ns_t_opt = 41,		// EDNS0 option (meta-RR)
	ns_t_apl = 42,		// Address prefix list (RFC3123)
	ns_t_tkey = 249,	// Transaction key
	ns_t_tsig = 250,	// Transaction signature
	ns_t_ixfr = 251,	// Incremental zone transfer
	ns_t_axfr = 252,	// Transfer zone of authority
	ns_t_mailb = 253,	// Transfer mailbox records
	ns_t_maila = 254,	// Transfer mail agent records
	ns_t_any = 255,		// Wildcard match
	ns_t_max = 65536
} ns_type;

extern uint16_t ns_getid(void *msg);
extern void ns_setid(void *msg, uint16_t id);
extern int ns_mkquery(void *buf, int buflen, const char *name, int type);
extern int ns_parse_query(void *msg, int len, char **dname, int *qtype);
extern int ns_parse_reply(void *msg, int len, char **dname, int *qtype);


#endif // DNS_H
