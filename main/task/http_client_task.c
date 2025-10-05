#include "http_client_task.h"
#include "http_client_app.h"
#include "wifi_app.h"
#include "oled_app.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

void http_client_task(void *pvParameters)
{
    const TickType_t REFRESH_INTERVAL = pdMS_TO_TICKS(5 * 60 * 1000);

    while (1)
    {
        EventBits_t bits = xEventGroupWaitBits(
            wifi_event_group,
            WIFI_GET_WEATHER_START_BIT,
            pdTRUE,
            pdFALSE,
            REFRESH_INTERVAL);

        // Nếu bit được set (người dùng yêu cầu fetch ngay)
        if (bits & WIFI_GET_WEATHER_START_BIT)
        {
            ESP_LOGI("http_client_task", "Fetch weather triggered by user");
            http_get_api_weather(&weatherData);
            xEventGroupSetBits(oled_event_group, OLED_WEATHER_START_BIT);
        }
        else
        {
            // Nếu timeout (5 phút) → tự động refresh
            ESP_LOGI("http_client_task", "Auto refresh weather (5 min interval)");
            http_get_api_weather(&weatherData);
            xEventGroupSetBits(oled_event_group, OLED_WEATHER_START_BIT);
        }
    }
}
