#include <string.h>
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_timer.h"
#include "dot.h"
#include "certs.h"
#include "config.h"

static const char *TAG = "dot";

esp_err_t dot_handshake_test(void)
{
    esp_tls_cfg_t cfg = {
        .cacert_buf       = (const unsigned char *)ca_crt_start,
        .cacert_bytes     = ca_crt_end - ca_crt_start,
        .clientcert_buf   = (const unsigned char *)client_crt_start,
        .clientcert_bytes = client_crt_end - client_crt_start,
        .clientkey_buf    = (const unsigned char *)client_key_start,
        .clientkey_bytes  = client_key_end - client_key_start,
        .common_name      = GATEWAY_NAME,
        .timeout_ms       = 5000,
    };

    esp_tls_t *tls = esp_tls_init();
    if (tls == NULL) {
        ESP_LOGE(TAG, "out of memory");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "connecting to %s:%d as %s", GATEWAY_IP, GATEWAY_PORT, GATEWAY_NAME);
    int64_t t0 = esp_timer_get_time();
    int ret = esp_tls_conn_new_sync(GATEWAY_IP, strlen(GATEWAY_IP), GATEWAY_PORT, &cfg, tls);
    int64_t ms = (esp_timer_get_time() - t0) / 1000;

    esp_err_t result;
    if (ret == 1) {
        ESP_LOGI(TAG, "mTLS handshake OK in %lld ms", ms);
        result = ESP_OK;
    } else {
        ESP_LOGE(TAG, "mTLS handshake FAILED after %lld ms", ms);
        result = ESP_FAIL;
    }
    esp_tls_conn_destroy(tls);
    return result;
}
