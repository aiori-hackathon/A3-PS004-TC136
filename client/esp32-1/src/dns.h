#pragma once
#include <stdint.h>
#include <stdbool.h>

#define DNS_TYPE_A         1
#define DNS_RCODE_NOERROR  0
#define DNS_RCODE_SERVFAIL 2

typedef struct {
    int     rcode;     // 0 NOERROR, 2 SERVFAIL, 3 NXDOMAIN ...
    bool    ad;        // gateway says: validated with DNSSEC
    bool    has_ip;
    uint8_t ip[4];     // first A record in the answer
    int     ede;       // Extended DNS Error code, -1 if none (for logs only)
} dns_result_t;

typedef enum {
    VERDICT_SECURE,     // NOERROR + AD: use the IP
    VERDICT_INSECURE,   // NOERROR, no AD: not signed, refuse for must-be-secure names
    VERDICT_BOGUS,      // SERVFAIL: validation failed (or upstream broken)
    VERDICT_NO_ANSWER,  // NXDOMAIN, no A record, other errors
} dns_verdict_t;

// build an A query for name into buf; returns its length or -1
int dns_build_query(uint16_t id, const char *name, uint8_t *buf, int cap);
// read a reply; returns 0 if it parsed and matches id
int dns_parse_reply(const uint8_t *msg, int len, uint16_t id, dns_result_t *r);

dns_verdict_t dns_verdict(const dns_result_t *r);
const char *dns_verdict_str(dns_verdict_t v);
