#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "client";

void app_main(void)
{
    int n = 0;
    while (1) {
        ESP_LOGI(TAG, "hello from esp32-1, tick %d", n++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
