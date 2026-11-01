#pragma once
#include <stdbool.h>
#include "dns.h"

// true if this name is on the must-be-secure list
bool policy_must_be_secure(const char *name);
// final decision: may the device use this answer?
bool policy_accept(const char *name, dns_verdict_t v);
