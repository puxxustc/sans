/*
* resolv.c
*/

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "resolv.h"

#define NS_TYPE_ELT             0x40 // EDNS0 extended label type
#define DNS_LABELTYPE_BITSTRING 0x41

static const char *digits = "0123456789";

static const char digitvalue[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 16
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 32
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 48
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, // 64
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 80
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 96
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 112
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 128
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 256
};

static int special(int);
static int printable(int);
static int dn_find(const u_char *, const u_char *,
                   const u_char * const *,
                   const u_char * const *);
static int encode_bitstring(const char **, const char *,
                            unsigned char **, unsigned char **,
                            unsigned const char *);
static int labellen(const u_char *);
static int decode_bitstring(const unsigned char **, char *, const char *);

/*
 * Convert an encoded domain name to printable ascii as per RFC1035.

 * return:
 *   Number of bytes written to buffer, or -1 (with errno set)
 *
 * notes:
 *   The root is returned as "."
 *   All other domains are returned in non absolute form
 */
int ns_name_ntop(const u_char *src, char *dst, size_t dstsiz)
{
    const u_char *cp;
    char *dn, *eom;
    u_char c;
    u_int n;
    int l;

    cp = src;
    dn = dst;
    eom = dst + dstsiz;

    while ((n = *cp++) != 0)
    {
        if ((n & NS_CMPRSFLGS) == NS_CMPRSFLGS)
        {
            // Some kind of compression pointer
            return -1;
        }
        if (dn != dst)
        {
            if (dn >= eom)
            {
                return -1;
            }
            *dn++ = '.';
        }
        if ((l = labellen(cp - 1)) < 0)
        {
            return -1;
        }
        if (dn + l >= eom)
        {
            return -1;
        }
        if ((n & NS_CMPRSFLGS) == NS_TYPE_ELT)
        {
            int m;

            if (n != DNS_LABELTYPE_BITSTRING)
            {
                // XXX: labellen should reject this case
                return -1;
            }
            if ((m = decode_bitstring(&cp, dn, eom)) < 0)
            {
                return -1;
            }
            dn += m;
            continue;
        }
        for (; l > 0; l--)
        {
            c = *cp++;
            if (special(c))
            {
                if (dn + 1 >= eom)
                {
                    return -1;
                }
                *dn++ = '\\';
                *dn++ = (char)c;
            }
            else if (!printable(c))
            {
                if (dn + 3 >= eom)
                {
                    return -1;
                }
                *dn++ = '\\';
                *dn++ = digits[c / 100];
                *dn++ = digits[(c % 100) / 10];
                *dn++ = digits[c % 10];
            }
            else
            {
                if (dn >= eom)
                {
                    return -1;
                }
                *dn++ = (char)c;
            }
        }
    }
    if (dn == dst)
    {
        if (dn >= eom)
        {
            return -1;
        }
        *dn++ = '.';
    }
    if (dn >= eom)
    {
        return -1;
    }
    *dn++ = '\0';
    return dn - dst;
}

/*
 * Convert an ascii string into an encoded domain name as per RFC1035.
 *
 * return:
 *   -1 if it fails
 *   1 if string was fully qualified
 *   0 is string was not fully qualified
 *
 * notes:
 *   Enforces label and domain length limits.
 */
int ns_name_pton(const char *src, u_char *dst, size_t dstsiz)
{
    u_char *label, *bp, *eom;
    int c, n, escaped, e = 0;
    char *cp;

    escaped = 0;
    bp = dst;
    eom = dst + dstsiz;
    label = bp++;

    while ((c = *src++) != 0)
    {
        if (escaped)
        {
            if (c == '[')
            {
                // start a bit string label
                if ((cp = strchr(src, ']')) == NULL)
                {
                    return -1;
                }
                if ((e = encode_bitstring(&src, cp + 2,
                                          &label, &bp, eom))
                        != 0)
                {
                    return -1;
                }
                escaped = 0;
                label = bp++;
                if ((c = *src++) == 0)
                {
                    goto done;
                }
                else if (c != '.')
                {
                    return -1;
                }
                continue;
            }
            else if ((cp = strchr(digits, c)) != NULL)
            {
                n = (cp - digits) * 100;
                if ((c = *src++) == 0 ||
                        (cp = strchr(digits, c)) == NULL)
                {
                    return -1;
                }
                n += (cp - digits) * 10;
                if ((c = *src++) == 0 ||
                        (cp = strchr(digits, c)) == NULL)
                {
                    return -1;
                }
                n += (cp - digits);
                if (n > 255)
                {
                    return -1;
                }
                c = n;
            }
            escaped = 0;
        } else if (c == '\\')
        {
            escaped = 1;
            continue;
        } else if (c == '.')
        {
            c = (bp - label - 1);
            if ((c & NS_CMPRSFLGS) != 0)
            {
                // Label too big
                return -1;
            }
            if (label >= eom)
            {
                return -1;
            }
            *label = c;
            // Fully qualified ?
            if (*src == '\0')
            {
                if (c != 0)
                {
                    if (bp >= eom)
                    {
                        return -1;
                    }
                    *bp++ = '\0';
                }
                if ((bp - dst) > NS_MAXCDNAME)
                {
                    return -1;
                }
                return 1;
            }
            if (c == 0 || *src == '.')
            {
                return -1;
            }
            label = bp++;
            continue;
        }
        if (bp >= eom)
        {
            return -1;
        }
        *bp++ = (u_char)c;
    }
    c = (bp - label - 1);
    if ((c & NS_CMPRSFLGS) != 0)
    {
        // Label too big
        return -1;
    }
done:
    if (label >= eom)
    {
        return -1;
    }
    *label = c;
    if (c != 0)
    {
        if (bp >= eom)
        {
            return -1;
        }
        *bp++ = 0;
    }
    if ((bp - dst) > NS_MAXCDNAME)
    {
        // src too big
        return -1;
    }
    return 0;
}

/*
 * Convert a network strings labels into all lowercase.
 *
 * return:
 *   Number of bytes written to buffer, or -1 (with errno set)
 *
 * notes:
 *   Enforces label and domain length limits.
 */
int ns_name_ntol(const u_char *src, u_char *dst, size_t dstsiz)
{
    const u_char *cp;
    u_char *dn, *eom;
    u_char c;
    u_int n;
    int l;

    cp = src;
    dn = dst;
    eom = dst + dstsiz;

    if (dn >= eom)
    {
        return -1;
    }
    while ((n = *cp++) != 0)
    {
        if ((n & NS_CMPRSFLGS) == NS_CMPRSFLGS)
        {
            /* Some kind of compression pointer. */
            return -1;
        }
        *dn++ = n;
        if ((l = labellen(cp - 1)) < 0)
        {
            return -1;
        }
        if (dn + l >= eom)
        {
            return -1;
        }
        for (; l > 0; l--)
        {
            c = *cp++;
            if (isupper(c))
            {
                *dn++ = tolower(c);
            }
            else
            {
                *dn++ = c;
            }
        }
    }
    *dn++ = '\0';
    return dn - dst;
}

/*
 * Unpack a domain name from a message, source may be compressed.
 *
 * return:
 *   -1 if it fails, or consumed octets if it succeeds.
 */
int ns_name_unpack(const u_char *msg, const u_char *eom, const u_char *src,
                   u_char *dst, size_t dstsiz)
{
    const u_char *srcp, *dstlim;
    u_char *dstp;
    int n, len, checked, l;

    len = -1;
    checked = 0;
    dstp = dst;
    srcp = src;
    dstlim = dst + dstsiz;
    if (srcp < msg || srcp >= eom)
    {
        return -1;
    }
    // Fetch next label in domain name
    while ((n = *srcp++) != 0)
    {
        // Check for indirection
        switch (n & NS_CMPRSFLGS)
        {
        case 0:
        case NS_TYPE_ELT:
        {
            // Limit checks
            if ((l = labellen(srcp - 1)) < 0)
            {
                return -1;
            }
            if (dstp + l + 1 >= dstlim || srcp + l >= eom)
            {
                return -1;
            }
            checked += l + 1;
            *dstp++ = n;
            memcpy(dstp, srcp, l);
            dstp += l;
            srcp += l;
            break;
        }
        case NS_CMPRSFLGS:
            if (srcp >= eom)
            {
                return -1;
            }
            if (len < 0)
            {
                len = srcp - src + 1;
            }
            srcp = msg + (((n & 0x3f) << 8) | (*srcp & 0xff));
            if (srcp < msg || srcp >= eom)
            {
                // Out of range
                return -1;
            }
            checked += 2;
            /*
             * Check for loops in the compressed name;
             * if we've looked at the whole message,
             * there must be a loop.
             */
            if (checked >= eom - msg)
            {
                return -1;
            }
            break;

        default:
            // flag error
            return -1;
        }
    }
    *dstp = '\0';
    if (len < 0)
    {
        len = srcp - src;
    }
    return len;
}

/*
 * Pack domain name 'domain' into 'comp_dn'.
 *
 * return:
 *   Size of the compressed name, or -1.
 *
 * notes:
 *   'dnptrs' is an array of pointers to previous compressed names.
 *   dnptrs[0] is a pointer to the beginning of the message. The array
 *   ends with NULL.
 *   'lastdnptr' is a pointer to the end of the array pointed to
 *   'dnptrs'.
 *
 * Side effects:
 *   The list of pointers in dnptrs is updated for labels inserted into
 *   the message as we compress the name.  If 'dnptr' is NULL, we don't
 *   try to compress names. If 'lastdnptr' is NULL, we don't update the
 *   list.
 */
int ns_name_pack(const u_char *src, u_char *dst, int dstsiz,
                 const u_char **dnptrs, const u_char **lastdnptr)
{
    u_char *dstp;
    const u_char **cpp, **lpp, *eob, *msg;
    const u_char *srcp;
    int n, l, first = 1;

    srcp = src;
    dstp = dst;
    eob = dstp + dstsiz;
    lpp = cpp = NULL;
    if (dnptrs != NULL)
    {
        if ((msg = *dnptrs++) != NULL)
        {
            for (cpp = dnptrs; *cpp != NULL; cpp++)
            {
            }
            // end of list to search
            lpp = cpp;
        }
    }
    else
    {
        msg = NULL;
    }

    // make sure the domain we are about to add is legal
    l = 0;
    do
    {
        int l0;

        n = *srcp;
        if ((n & NS_CMPRSFLGS) == NS_CMPRSFLGS)
        {
            return -1;
        }
        if ((l0 = labellen(srcp)) < 0)
        {
            return -1;
        }
        l += l0 + 1;
        if (l > NS_MAXCDNAME)
        {
            return -1;
        }
        srcp += l0 + 1;
    } while (n != 0);

    // from here on we need to reset compression pointer array on error
    srcp = src;
    do
    {
        // Look to see if we can use pointers. */
        n = *srcp;
        if (n != 0 && msg != NULL)
        {
            l = dn_find(srcp, msg, (const u_char * const *)dnptrs,
                        (const u_char * const *)lpp);
            if (l >= 0)
            {
                if (dstp + 1 >= eob)
                {
                    goto cleanup;
                }
                *dstp++ = (l >> 8) | NS_CMPRSFLGS;
                *dstp++ = l % 256;
                return dstp - dst;
            }
            /* Not found, save it. */
            if (lastdnptr != NULL && cpp < lastdnptr - 1 &&
                    (dstp - msg) < 0x4000 && first)
            {
                *cpp++ = dstp;
                *cpp = NULL;
                first = 0;
            }
        }
        /* copy label to buffer */
        if ((n & NS_CMPRSFLGS) == NS_CMPRSFLGS)
        {
            /* Should not happen. */
            goto cleanup;
        }
        n = labellen(srcp);
        if (dstp + 1 + n >= eob)
        {
            goto cleanup;
        }
        memcpy(dstp, srcp, n + 1);
        srcp += n + 1;
        dstp += n + 1;
    } while (n != 0);

    if (dstp > eob)
    {
cleanup:
        if (msg != NULL)
            *lpp = NULL;
        return -1;
    }
    return dstp - dst;
}

/*
 * Expand compressed domain name to presentation format.
 *
 * return:
 *   Number of bytes read out of `src', or -1 (with errno set).
 *
 * note:
 *   Root domain returns as "." not "".
 */
int ns_name_uncompress(const u_char *msg, const u_char *eom, const u_char *src,
                       char *dst, size_t dstsiz)
{
    u_char tmp[NS_MAXCDNAME];
    int n;

    if ((n = ns_name_unpack(msg, eom, src, tmp, sizeof tmp)) == -1)
    {
        return -1;
    }
    if (ns_name_ntop(tmp, dst, dstsiz) == -1)
    {
        return -1;
    }
    return n;
}

/*
 *  Compress a domain name into wire format, using compression pointers.
 *
 * return:
 *  Number of bytes consumed in `dst' or -1 (with errno set).
 *
 * notes:
 *  'dnptrs' is an array of pointers to previous compressed names.
 *  dnptrs[0] is a pointer to the beginning of the message.
 *  The list ends with NULL.  'lastdnptr' is a pointer to the end of the
 *  array pointed to by 'dnptrs'. Side effect is to update the list of
 *  pointers for labels inserted into the message as we compress the name.
 *  If 'dnptr' is NULL, we don't try to compress names. If 'lastdnptr'
 *  is NULL, we don't update the list.
 */
int ns_name_compress(const char *src, u_char *dst, size_t dstsiz,
                     const u_char **dnptrs, const u_char **lastdnptr)
{
    u_char tmp[NS_MAXCDNAME];

    if (ns_name_pton(src, tmp, sizeof(tmp)) == -1)
    {
        return -1;
    }
    return ns_name_pack(tmp, dst, dstsiz, dnptrs, lastdnptr);
}

/*
 * Reset dnptrs so that there are no active references to pointers at or
 * after src.
 */
void ns_name_rollback(const u_char *src, const u_char **dnptrs,
                      const u_char **lastdnptr)
{
    while (dnptrs < lastdnptr && *dnptrs != NULL)
    {
        if (*dnptrs >= src)
        {
            *dnptrs = NULL;
            break;
        }
        dnptrs++;
    }
}

/*
 *  Advance *ptrptr to skip over the compressed name it points at.
 *
 * return:
 *   0 on success, -1 (with errno set) on failure.
 */
int ns_name_skip(const u_char **ptrptr, const u_char *eom)
{
    const u_char *cp;
    u_int n;
    int l;

    cp = *ptrptr;
    while (cp < eom && (n = *cp++) != 0)
    {
        // Check for indirection
        switch (n & NS_CMPRSFLGS)
        {
        case 0:
            // normal case, n == len
            cp += n;
            continue;
        case NS_TYPE_ELT:
            // EDNS0 extended label
            if ((l = labellen(cp - 1)) < 0)
            {
                return -1;
            }
            cp += l;
            continue;
        case NS_CMPRSFLGS:
            // indirection
            cp++;
            break;
        default:
            // illegal type
            return -1;
        }
        break;
    }
    if (cp > eom)
    {
        return -1;
    }
    *ptrptr = cp;
    return 0;
}

/*
 *  Thinking in noninternationalized USASCII (per the DNS spec),
 *  is this character special ("in need of quoting") ?
 *
 * return:
 *   boolean.
 */
static int special(int ch)
{
    switch (ch)
    {
    case '"':
    case '.':
    case ';':
    case '\\':
    case '(':
    case ')':
    // Special modifiers in zone files
    case '@':
    case '$':
        return 1;
    default:
        return 0;
    }
}

static int printable(int ch)
{
    return (ch > 0x20) && (ch < 0x7f);
}

static int mklower(int ch)
{
    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return ch + 0x20;
    }
    else
    {
        return ch;
    }
}

/*
 * Search for the counted-label name in an array of compressed names.
 *
 * return:
 *   offset from msg if found, or -1.
 *
 * notes:
 *   dnptrs is the pointer to the first name on the list,
 *   not the pointer to the start of the message.
 */
static int dn_find(const u_char *domain, const u_char *msg,
                   const u_char * const *dnptrs,
                   const u_char * const *lastdnptr)
{
    const u_char *dn, *cp, *sp;
    const u_char * const *cpp;
    u_int n;

    for (cpp = dnptrs; cpp < lastdnptr; cpp++)
    {
        sp = *cpp;
        /*
         * terminate search on:
         * root label
         * compression pointer
         * unusable offset
         */
        while (   (*sp != 0)
               && ((*sp & NS_CMPRSFLGS) == 0)
               && ((sp - msg) < 0x4000))
        {
            dn = domain;
            cp = sp;
            while ((n = *cp++) != 0)
            {
                // check for indirection
                switch (n & NS_CMPRSFLGS)
                {
                case 0:
                {
                    // normal case, n == len
                    n = labellen(cp - 1); /*%< XXX */
                    if (n != *dn++)
                        goto next;

                    for (; n > 0; n--)
                    {
                        if (mklower(*dn++) != mklower(*cp++))
                        {
                            goto next;
                        }
                    }
                    /* Is next root for both ? */
                    if (*dn == '\0' && *cp == '\0')
                    {
                        return sp - msg;
                    }
                    if (*dn)
                    {
                        continue;
                    }
                    goto next;
                }
                case NS_CMPRSFLGS:
                {
                    // indirection
                    cp = msg + (((n & 0x3f) << 8) | *cp);
                    break;
                }
                default:
                    // illegal type
                    return -1;
                }
            }
next:
            sp += *sp + 1;
        }
    }
    return -1;
}

static int decode_bitstring(const unsigned char **cpp, char *dn, const char *eom)
{
    const unsigned char *cp = *cpp;
    char *beg = dn, tc;
    int b, blen, plen, i;

    if ((blen = (*cp & 0xff)) == 0)
        blen = 256;
    plen = (blen + 3) / 4;
    plen += sizeof("\\[x/]") + (blen > 99 ? 3 : (blen > 9) ? 2 : 1);
    if (dn + plen >= eom)
        return -1;

    cp++;
    i = sprintf(dn, "\\[x");
    if (i < 0)
        return -1;
    dn += i;
    for (b = blen; b > 7; b -= 8, cp++)
    {
        i = sprintf(dn, "%02x", *cp & 0xff);
        if (i < 0)
        {
            return -1;
        }
        dn += i;
    }
    if (b > 4)
    {
        tc = *cp++;
        i = sprintf(dn, "%02x", tc & (0xff << (8 - b)));
        if (i < 0)
        {
            return -1;
        }
        dn += i;
    } else if (b > 0)
    {
        tc = *cp++;
        i = sprintf(dn, "%1x", ((tc >> 4) & 0x0f) & (0x0f << (4 - b)));
        if (i < 0)
        {
            return -1;
        }
        dn += i;
    }
    i = sprintf(dn, "/%d]", blen);
    if (i < 0)
    {
        return -1;
    }
    dn += i;

    *cpp = cp;
    return dn - beg;
}

static int encode_bitstring(const char **bp, const char *end,
                            unsigned char **labelp, unsigned char ** dst,
                            unsigned const char *eom)
{
    int afterslash = 0;
    const char *cp = *bp;
    unsigned char *tp;
    char c;
    const char *beg_blen;
    char *end_blen = NULL;
    int value = 0, count = 0, tbcount = 0, blen = 0;

    beg_blen = end_blen = NULL;

    // a bitstring must contain at least 2 characters
    if (end - cp < 2)
    {
        return -1;
    }

    /* XXX: currently, only hex strings are supported */
    if (*cp++ != 'x')
    {
        return -1;
    }
    if (!isxdigit((*cp) & 0xff))
    {
        // reject '\[x/BLEN]'
        return -1;
    }

    for (tp = *dst + 1; cp < end && tp < eom; cp++)
    {
        switch((c = *cp))
        {
        case ']':
            // end of the bitstring
            if (afterslash)
            {
                if (beg_blen == NULL)
                {
                    return -1;
                }
                blen = (int)strtol(beg_blen, &end_blen, 10);
                if (*end_blen != ']')
                {
                    return -1;
                }
            }
            if (count)
            {
                *tp++ = ((value << 4) & 0xff);
            }
            cp++;   // skip ']'
            goto done;
        case '/':
            afterslash = 1;
            break;
        default:
            if (afterslash)
            {
                if (!isdigit(c&0xff))
                {
                    return -1;
                }
                if (beg_blen == NULL)
                {

                    if (c == '0')
                    {
                        // blen never begings with 0
                        return -1;
                    }
                    beg_blen = cp;
                }
            }
            else
            {
                if (!isxdigit(c&0xff))
                {
                    return -1;
                }
                value <<= 4;
                value += digitvalue[(int)c];
                count += 4;
                tbcount += 4;
                if (tbcount > 256)
                {
                    return -1;
                }
                if (count == 8)
                {
                    *tp++ = value;
                    count = 0;
                }
            }
            break;
        }
    }
done:
    if (cp >= end || tp >= eom)
    {
        return -1;
    }

    /*
     * bit length validation:
     * If a <length> is present, the number of digits in the <bit-data>
     * MUST be just sufficient to contain the number of bits specified
     * by the <length>. If there are insignificant bits in a final
     * hexadecimal or octal digit, they MUST be zero.
     * RFC2673, Section 3.2.
     */
    if (blen > 0)
    {
        int traillen;

        if (((blen + 3) & ~3) != tbcount)
        {
            return -1;
        }
        traillen = tbcount - blen; // between 0 and 3
        if (((value << (8 - traillen)) & 0xff) != 0)
        {
            return -1;
        }
    }
    else
    {
        blen = tbcount;
    }
    if (blen == 256)
    {
        blen = 0;
    }

    // encode the type and the significant bit fields
    **labelp = DNS_LABELTYPE_BITSTRING;
    **dst = blen;

    *bp = cp;
    *dst = tp;

    return 0;
}

static int labellen(const u_char *lp)
{
    int bitlen;
    u_char l = *lp;

    if ((l & NS_CMPRSFLGS) == NS_CMPRSFLGS)
    {
        // should be avoided by the caller
        return -1;
    }

    if ((l & NS_CMPRSFLGS) == NS_TYPE_ELT)
    {
        if (l == DNS_LABELTYPE_BITSTRING)
        {
            if ((bitlen = *(lp + 1)) == 0)
            {
                bitlen = 256;
            }
            return (bitlen + 7 ) / 8 + 1;
        }
        return -1;  // unknwon ELT
    }
    return l;
}


static void setsection(ns_msg *msg, ns_sect sect);

int ns_skiprr(const void *ptr, const void *eom, ns_sect section, int count)
{
    const void *optr = ptr;

    for (; count > 0; count--)
    {
        int b, rdlength;

        b = dn_skipname(ptr, eom);
        if (b < 0)
            return -1;
        ptr += b /*Name*/ + NS_INT16SZ /*Type*/ + NS_INT16SZ /*Class*/;
        if (section != ns_s_qd)
        {
            if (ptr + NS_INT32SZ + NS_INT16SZ > eom)
                return -1;
            ptr += NS_INT32SZ; // TTL
            NS_GET16(rdlength, ptr);
            ptr += rdlength; // RData
        }
    }
    if (ptr > eom)
    {
        return -1;
    }
    return ptr - optr;
}

int ns_initparse(const void *msg, int msglen, ns_msg *handle)
{
    const void *eom = msg + msglen;

    memset(handle, 0x5e, sizeof *handle);
    handle->msg = msg;
    handle->eom = eom;
    if (msg + NS_INT16SZ > eom)
    {
        return -1;
    }
    NS_GET16(handle->id, msg);
    if (msg + NS_INT16SZ > eom)
    {
        return -1;
    }
    NS_GET16(handle->flags, msg);
    for (int i = 0; i < ns_s_max; i++)
    {
        if (msg + NS_INT16SZ > eom)
        {
            return -1;
        }
        NS_GET16(handle->counts[i], msg);
    }
    for (int i = 0; i < ns_s_max; i++)
    {
        if (handle->counts[i] == 0)
        {
            handle->sections[i] = NULL;
        }
        else
        {
            int b = ns_skiprr(msg, eom, (ns_sect)i,
                              handle->counts[i]);

            if (b < 0)
            {
                return -1;
            }
            handle->sections[i] = msg;
            msg += b;
        }
    }
    if (msg != eom)
    {
        return -1;
    }
    setsection(handle, ns_s_max);
    return 0;
}

int ns_parserr(ns_msg *handle, ns_sect section, int rrnum, ns_rr *rr)
{
    int b;
    int tmp;

    // Make section right
    tmp = section;
    if (tmp < 0 || section >= ns_s_max)
    {
        return -1;
    }
    if (section != handle->sect)
    {
        setsection(handle, section);
    }

    // Make rrnum right
    if (rrnum == -1)
    {
        rrnum = handle->rrnum;
    }
    if (rrnum < 0 || rrnum >= handle->counts[(int)section])
    {
        return -1;
    }
    if (rrnum < handle->rrnum)
    {
        setsection(handle, section);
    }
    if (rrnum > handle->rrnum)
    {
        b = ns_skiprr(handle->msg_ptr, handle->eom, section,
                      rrnum - handle->rrnum);

        if (b < 0)
        {
            return -1;
        }
        handle->msg_ptr += b;
        handle->rrnum = rrnum;
    }

    // do the parse
    b = dn_expand(handle->msg, handle->eom,
                  handle->msg_ptr, rr->name, NS_MAXDNAME);
    if (b < 0)
    {
        return -1;
    }
    handle->msg_ptr += b;
    if (handle->msg_ptr + NS_INT16SZ + NS_INT16SZ > handle->eom)
    {
        return -1;
    }
    NS_GET16(rr->type, handle->msg_ptr);
    handle->msg_ptr += 2;   // skip class
    if (section == ns_s_qd)
    {
        rr->ttl = 0;
        rr->rdlength = 0;
        rr->rdata = NULL;
    }
    else
    {
        if (handle->msg_ptr + NS_INT32SZ + NS_INT16SZ > handle->eom)
        {
            return -1;
        }
        NS_GET32(rr->ttl, handle->msg_ptr);
        NS_GET16(rr->rdlength, handle->msg_ptr);
        if (handle->msg_ptr + rr->rdlength > handle->eom)
        {
            return -1;
        }
        rr->rdata = handle->msg_ptr;
        handle->msg_ptr += rr->rdlength;
    }
    if (++handle->rrnum > handle->counts[(int)section])
    {
        setsection(handle, (ns_sect)((int)section + 1));
    }

    return 0;
}

static void setsection(ns_msg *msg, ns_sect sect)
{
    msg->sect = sect;
    if (sect == ns_s_max)
    {
        msg->rrnum = -1;
        msg->msg_ptr = NULL;
    }
    else
    {
        msg->rrnum = 0;
        msg->msg_ptr = msg->sections[(int)sect];
    }
}


/*
 * Expand compressed domain name 'comp_dn' to full domain name.
 * 'msg' is a pointer to the beginning of the message,
 * 'eomorig' points to the first location after the message,
 * 'exp_dn' is a pointer to a buffer of size 'length' for the result.
 * Return size of compressed name or -1 if there was an error.
 */
int dn_expand(const u_char *msg, const u_char *eom, const u_char *src,
              char *dst, int dstsiz)
{
    int n = ns_name_uncompress(msg, eom, src, dst, (size_t)dstsiz);

    if (n > 0 && dst[0] == '.')
    {
        dst[0] = '\0';
    }
    return n;
}

/*
 * Pack domain name 'exp_dn' in presentation form into 'comp_dn'.
 * Return the size of the compressed name or -1.
 * 'length' is the size of the array pointed to by 'comp_dn'.
 */
int dn_comp(const char *src, u_char *dst, int dstsiz,
            u_char **dnptrs, u_char **lastdnptr)
{
    return ns_name_compress(src, dst, (size_t)dstsiz,
                            (const u_char **)dnptrs,
                            (const u_char **)lastdnptr);
}

/*
 * Skip over a compressed domain name. Return the size or -1.
 */
int dn_skipname(const u_char *ptr, const u_char *eom)
{
    const u_char *saveptr = ptr;

    if (ns_name_skip(&ptr, eom) == -1)
    {
        return -1;
    }
    return ptr - saveptr;
}

