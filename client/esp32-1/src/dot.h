#pragma once
#include "esp_err.h"
#include "dns.h"

// open one mTLS connection to the gateway and keep it
esp_err_t dot_connect(void);
// ask for the A record of name over that connection
esp_err_t dot_query(const char *name, dns_result_t *out);
void dot_close(void);
