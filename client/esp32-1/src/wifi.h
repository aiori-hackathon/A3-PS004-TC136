#pragma once
#include "esp_err.h"

// connect to the Wi-Fi in secrets.h, wait until we have an IP
esp_err_t wifi_connect(void);
