#include <string.h>
#include "dns.h"

#define FLAG_RD 0x0100   // recursion desired
#define FLAG_AD 0x0020   // authentic data
#define TYPE_OPT 41
#define EDE_OPTION 15

static void put16(uint8_t *p, uint16_t v) { p[0] = v >> 8; p[1] = v & 0xFF; }
static uint16_t get16(const uint8_t *p) { return (p[0] << 8) | p[1]; }

int dns_build_query(uint16_t id, const char *name, uint8_t *buf, int cap)
{
    if (cap < 12) {
        return -1;
    }
    // header: id, flags, 1 question, 0 answers, 0 authority, 1 additional (OPT)
    put16(buf + 0, id);
    put16(buf + 2, FLAG_RD | FLAG_AD);   // AD in a query = "tell me if you validated" (RFC 6840)
    put16(buf + 4, 1);
    put16(buf + 6, 0);
    put16(buf + 8, 0);
    put16(buf + 10, 1);
    int n = 12;

    // name as labels: "ota.lab.test" -> 3 ota 3 lab 4 test 0
    const char *p = name;
    while (*p) {
        const char *dot = strchr(p, '.');
        int len = dot ? (int)(dot - p) : (int)strlen(p);
        if (len == 0 || len > 63 || n + 1 + len >= cap) {
            return -1;
        }
        buf[n++] = len;
        memcpy(buf + n, p, len);
        n += len;
        p += len;
        if (*p == '.') {
            p++;
        }
    }
    if (n + 1 + 4 + 11 > cap) {
        return -1;
    }
    buf[n++] = 0;
    put16(buf + n, DNS_TYPE_A); n += 2;
    put16(buf + n, 1);          n += 2;   // class IN

    // EDNS OPT record, so the gateway can send Extended DNS Errors back
    buf[n++] = 0;                         // name: root
    put16(buf + n, TYPE_OPT);   n += 2;
    put16(buf + n, 1232);       n += 2;   // max reply size
    put16(buf + n, 0);          n += 2;   // extended rcode + version
    put16(buf + n, 0);          n += 2;   // flags: no DO, we don't want the signatures
    put16(buf + n, 0);          n += 2;   // no options
    return n;
}

// offset just after a name (labels or a compression pointer), or -1
static int skip_name(const uint8_t *m, int len, int off)
{
    while (off < len) {
        uint8_t c = m[off];
        if (c == 0) {
            return off + 1;
        }
        if ((c & 0xC0) == 0xC0) {
            return off + 2 <= len ? off + 2 : -1;
        }
        off += 1 + c;
    }
    return -1;
}

int dns_parse_reply(const uint8_t *m, int len, uint16_t id, dns_result_t *r)
{
    memset(r, 0, sizeof(*r));
    r->ede = -1;
    if (len < 12 || get16(m) != id) {
        return -1;
    }
    uint16_t flags = get16(m + 2);
    r->rcode = flags & 0x000F;
    r->ad = (flags & FLAG_AD) != 0;

    int qd = get16(m + 4), an = get16(m + 6), ns = get16(m + 8), ar = get16(m + 10);
    int off = 12;
    for (int i = 0; i < qd; i++) {
        off = skip_name(m, len, off);
        if (off < 0 || off + 4 > len) {
            return -1;
        }
        off += 4;
    }
    // every record: name, type(2) class(2) ttl(4) rdlen(2), rdata
    for (int i = 0; i < an + ns + ar; i++) {
        off = skip_name(m, len, off);
        if (off < 0 || off + 10 > len) {
            return -1;
        }
        uint16_t type = get16(m + off);
        uint16_t rdlen = get16(m + off + 8);
        const uint8_t *rd = m + off + 10;
        if (off + 10 + rdlen > len) {
            return -1;
        }
        if (i < an && type == DNS_TYPE_A && rdlen == 4 && !r->has_ip) {
            memcpy(r->ip, rd, 4);
            r->has_ip = true;
        }
        if (i >= an + ns && type == TYPE_OPT) {
            // options inside OPT: code(2) len(2) data; EDE data starts with the info code
            int o = 0;
            while (o + 4 <= rdlen) {
                uint16_t code = get16(rd + o), olen = get16(rd + o + 2);
                if (o + 4 + olen > rdlen) {
                    break;
                }
                if (code == EDE_OPTION && olen >= 2) {
                    r->ede = get16(rd + o + 4);
                }
                o += 4 + olen;
            }
        }
        off += 10 + rdlen;
    }
    return 0;
}

dns_verdict_t dns_verdict(const dns_result_t *r)
{
    if (r->rcode == DNS_RCODE_SERVFAIL) {
        return VERDICT_BOGUS;
    }
    if (r->rcode != DNS_RCODE_NOERROR || !r->has_ip) {
        return VERDICT_NO_ANSWER;
    }
    return r->ad ? VERDICT_SECURE : VERDICT_INSECURE;
}

const char *dns_verdict_str(dns_verdict_t v)
{
    switch (v) {
    case VERDICT_SECURE:   return "SECURE";
    case VERDICT_INSECURE: return "INSECURE";
    case VERDICT_BOGUS:    return "BOGUS";
    default:               return "NO ANSWER";
    }
}
