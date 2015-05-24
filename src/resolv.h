/*
* resolv.h
*/

#ifndef RESOLV_H
#define RESOLV_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __MINGW32__
#  include "win.h"
#else
#  include <netinet/in.h>
#  include <arpa/inet.h>
#endif

// Define constants based on RFC 883, RFC 1034, RFC 1035
//#define NS_PACKETSZ     512     // default UDP packet size
#define NS_MAXDNAME     1025    // maximum domain name
#define NS_MAXMSG       65535   // maximum message size
#define NS_MAXCDNAME    255     // maximum compressed domain name
#define NS_MAXLABEL     63      // maximum length of domain label
#define NS_HFIXEDSZ     12      // #/bytes of fixed data in header
#define NS_QFIXEDSZ     4       // #/bytes of fixed data in query
#define NS_RRFIXEDSZ    10      // #/bytes of fixed data in r record
#define NS_INT32SZ      4       // #/bytes of data in a uint32_t
#define NS_INT16SZ      2       // #/bytes of data in a uint16_t
#define NS_INT8SZ       1       // #/bytes of data in a u_int8_t
#define NS_INADDRSZ     4       // IPv4 T_A
#define NS_IN6ADDRSZ    16      // IPv6 T_AAAA
#define NS_CMPRSFLGS    0xc0    // Flag bits indicating name compression

typedef enum
{
    ns_s_qd = 0,    // Question
    ns_s_an = 1,    // Answer
    ns_s_ns = 2,    // Name servers
    ns_s_ar = 3,    // Additional records
    ns_s_max = 4
} ns_sect;

// This is a message handle. It is caller allocated and has no dynamic data.
typedef struct
{
    const u_char *msg, *eom;
    uint16_t id;
    uint16_t flags;
    uint16_t counts[ns_s_max];
    const u_char *sections[ns_s_max];
    ns_sect sect;
    int rrnum;
    const u_char *msg_ptr;
} ns_msg;

#define ns_msg_id(handle) ((handle).id + 0)
#define ns_msg_base(handle) ((handle).msg + 0)
#define ns_msg_end(handle) ((handle).eom + 0)
#define ns_msg_size(handle) ((handle).eom - (handle).msg)
#define ns_msg_count(handle, section) ((handle).counts[section] + 0)
#define ns_msg_flag(handle) ((handle).flags)

// This is a parsed record.  It is caller allocated and has no dynamic data.
typedef struct
{
    char name[NS_MAXDNAME];
    uint32_t ttl;
    uint16_t type;
    uint16_t rdlength;
    const void *rdata;
} ns_rr;

// Accessor macros
#define ns_rr_name(rr)  (((rr).name[0] != '\0') ? (rr).name : ".")
#define ns_rr_type(rr)  ((ns_type)((rr).type + 0))
#define ns_rr_class(rr) ((ns_class)((rr).rr_class + 0))
#define ns_rr_ttl(rr)   ((rr).ttl + 0)
#define ns_rr_rdlen(rr) ((rr).rdlength + 0)
#define ns_rr_rdata(rr) ((rr).rdata + 0)

// inline versions of get/put short/long. Pointer is advanced.
#define NS_GET16(s, cp) do { \
    const u_char *t_cp = (const u_char *)(cp); \
    (s) = ((uint16_t)t_cp[0] << 8) \
        | ((uint16_t)t_cp[1]) \
        ; \
    (cp) += NS_INT16SZ; \
} while (0)

#define NS_GET32(l, cp) do { \
    const u_char *t_cp = (const u_char *)(cp); \
    (l) = ((uint32_t)t_cp[0] << 24) \
        | ((uint32_t)t_cp[1] << 16) \
        | ((uint32_t)t_cp[2] << 8) \
        | ((uint32_t)t_cp[3]) \
        ; \
    (cp) += NS_INT32SZ; \
} while (0)

#define NS_PUT16(s, cp) do { \
    uint16_t t_s = (uint16_t)(s); \
    u_char *t_cp = (u_char *)(cp); \
    *t_cp++ = t_s >> 8; \
    *t_cp   = t_s; \
    (cp) += NS_INT16SZ; \
} while (0)

#define NS_PUT32(l, cp) do { \
    uint32_t t_l = (uint32_t)(l); \
    u_char *t_cp = (u_char *)(cp); \
    *t_cp++ = t_l >> 24; \
    *t_cp++ = t_l >> 16; \
    *t_cp++ = t_l >> 8; \
    *t_cp   = t_l; \
    (cp) += NS_INT32SZ; \
} while (0)

int ns_initparse(const void *, int, ns_msg *);
int ns_skiprr(const void *, const void *, ns_sect, int);
int ns_parserr(ns_msg *, ns_sect, int, ns_rr *);
int ns_name_ntol(const u_char *, u_char *, size_t);
int ns_name_ntop(const u_char *, char *, size_t);
int ns_name_pton(const char *, u_char *, size_t);
int ns_name_unpack(const u_char *, const u_char *,
                   const u_char *, u_char *, size_t);
int ns_name_pack(const u_char *, u_char *, int,
                 const u_char **, const u_char **);
int ns_name_uncompress(const u_char *, const u_char *,
                       const u_char *, char *, size_t);
int ns_name_compress(const char *, u_char *, size_t,
                     const u_char **, const u_char **);
int ns_name_skip(const u_char **, const u_char *);
void ns_name_rollback(const u_char *, const u_char **, const u_char **);

extern int dn_skipname(const u_char *, const u_char *);
extern int dn_comp(const char *, u_char *, int, u_char **, u_char **);
extern int dn_expand(const u_char *, const u_char *, const u_char *,
                     char *, int);

#endif // RESOLV_H
