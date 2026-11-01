// sntp env: the clock is wrong after every boot (1970), so certificate dates cannot be checked until
// it is set. Ask NTP_SERVER_IP for the time before the TLS connect.
#ifdef NTP_SERVER_IP

#include <time.h>
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "timesync.h"

static const char *TAG = "time";

esp_err_t time_sync(void)
{
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER_IP);
    esp_err_t ret = esp_netif_sntp_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sntp init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "asking %s for the time", NTP_SERVER_IP);
    ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000));
    esp_netif_sntp_deinit();

    if (ret != ESP_OK) {
        // the clock stays at 1970: the gateway certificate will be "not yet valid"
        ESP_LOGW(TAG, "no time from %s: expect the TLS handshake to fail on the certificate dates", NTP_SERVER_IP);
        return ret;
    }

    time_t now = time(NULL);
    struct tm tm;
    char buf[32];
    gmtime_r(&now, &tm);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    ESP_LOGI(TAG, "clock set: %s UTC", buf);
    return ESP_OK;
}

#endif /* NTP_SERVER_IP */
