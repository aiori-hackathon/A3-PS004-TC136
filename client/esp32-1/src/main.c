#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "dot.h"
#include "policy.h"

static const char *TAG = "client";

// one name per test zone
static const char *names[] = {
    "ota.lab.test",
    "ota.signed.lab.test",
    "ota.unsigned.lab.test",
    "ota.expired.lab.test",
    "ota.future.lab.test",
    "ota.badsig.lab.test",
    "ota.unknownalg.lab.test",
    "ota.nosig.lab.test",
    "ota.nokey.lab.test",
};

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

    if (dot_connect() != ESP_OK) {
        ESP_LOGE(TAG, "no gateway, stopping");
        return;
    }

    // all questions over the same TLS connection: the handshake is paid once
    for (int i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        dns_result_t r;
        int64_t t0 = esp_timer_get_time();
        if (dot_query(names[i], &r) != ESP_OK) {
            ESP_LOGE(TAG, "%-24s query failed", names[i]);
            break;
        }
        int64_t ms = (esp_timer_get_time() - t0) / 1000;
        dns_verdict_t v = dns_verdict(&r);

        if (policy_accept(names[i], v)) {
            ESP_LOGI(TAG, "%-24s %-9s use %d.%d.%d.%d  (rcode %d, ad %d, %lld ms)",
                     names[i], dns_verdict_str(v), r.ip[0], r.ip[1], r.ip[2], r.ip[3],
                     r.rcode, r.ad, ms);
        } else if (v == VERDICT_INSECURE) {
            ESP_LOGW(TAG, "%-24s %-9s refused: must be secure  (rcode %d, ad %d, %lld ms)",
                     names[i], dns_verdict_str(v), r.rcode, r.ad, ms);
        } else {
            ESP_LOGW(TAG, "%-24s %-9s refused  (rcode %d, ede %d, %lld ms)",
                     names[i], dns_verdict_str(v), r.rcode, r.ede, ms);
        }
    }
    dot_close();
}
