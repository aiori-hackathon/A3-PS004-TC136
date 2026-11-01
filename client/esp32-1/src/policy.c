#include <string.h>
#include "policy.h"

// names the device must never use without DNSSEC (e.g. where firmware updates come from)
static const char *must_be_secure[] = {
    "ota.lab.test",
    "ota.signed.lab.test",
    "ota.unsigned.lab.test",
    "ota.unknownalg.lab.test",
};

bool policy_must_be_secure(const char *name)
{
    for (int i = 0; i < sizeof(must_be_secure) / sizeof(must_be_secure[0]); i++) {
        if (strcmp(name, must_be_secure[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool policy_accept(const char *name, dns_verdict_t v)
{
    if (v == VERDICT_SECURE) {
        return true;
    }
    if (v == VERDICT_INSECURE) {
        // unsigned answer: fine for ordinary names, a downgrade for critical ones
        return !policy_must_be_secure(name);
    }
    return false;
}
