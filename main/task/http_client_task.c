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
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_GET_WEATHER_START_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    xEventGroupSetBits(oled_event_group, WIFI_GET_WEATHER_CLK_BIT);
    while (1)
    {
        bits = xEventGroupWaitBits(
            wifi_event_group,
            WIFI_GET_WEATHER_CLK_BIT,
            pdTRUE,
            pdFALSE,
            REFRESH_INTERVAL);

        if (bits & WIFI_GET_WEATHER_CLK_BIT)
        {
            ESP_LOGI("http_client_task", "Fetch weather triggered by user");
            http_get_api_weather(&weatherData);
            xEventGroupSetBits(oled_event_group, OLED_WEATHER_START_BIT);
        }
        else
        {
            ESP_LOGI("http_client_task", "Auto refresh weather (5 min interval)");
            http_get_api_weather(&weatherData);
            xEventGroupSetBits(oled_event_group, OLED_WEATHER_START_BIT);
        }
    }
}
