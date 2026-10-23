#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "certs.h"

static const char *TAG = "client";

void app_main(void)
{
    // Wi-Fi keeps its calibration data in NVS (a small storage area in flash)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (wifi_connect() != ESP_OK) {
        ESP_LOGE(TAG, "no wifi, stopping");
        return;
    }
    ESP_LOGI(TAG, "wifi ready");

    // embedded certs: print sizes only, never the key itself
    ESP_LOGI(TAG, "ca.crt %d bytes, esp32-1.crt %d bytes, esp32-1.key %d bytes",
             (int)(ca_crt_end - ca_crt_start),
             (int)(client_crt_end - client_crt_start),
             (int)(client_key_end - client_key_start));
}
