#pragma once
#include "esp_err.h"

// set the clock over SNTP from NTP_SERVER_IP (an address, so no DNS is needed).
// only called in the sntp env, where mbedTLS also checks certificate dates
esp_err_t time_sync(void);
