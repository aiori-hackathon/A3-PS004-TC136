#pragma once
#include "esp_err.h"

// open an mTLS connection to the gateway, then close it
esp_err_t dot_handshake_test(void);
