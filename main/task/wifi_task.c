#include "esp_err.h"
#include "wifi_app.h"
#include "oled_app.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
static const char *TAG_WIFI = "wifi_app";

void wifi_task(void *pvParameters)
{
    wifi_init();
    wifi_init_apsta();
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_AP_START_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    while (1)
    {
        bits = xEventGroupWaitBits(wifi_event_group,
                                   WIFI_SCAN_START_BIT | WIFI_SCAN_DONE_BIT |
                                       WIFI_CONNECT_START_BIT | WIFI_CONNECT_DONE_BIT |
                                       WIFI_DISCONNECT_START_BIT | WIFI_DISCONNECT_DONE_BIT |
                                       WIFI_AP_STACONNECTED_BIT,
                                   pdTRUE,
                                   pdFALSE,
                                   portMAX_DELAY);
        if (bits & WIFI_SCAN_START_BIT)
        {
            xEventGroupSetBits(oled_event_group, OLED_SCAN_START_BIT);
            xEventGroupClearBits(wifi_event_group, WIFI_GET_SCAN_DONE_BIT);
            wifi_scan_start();
        }
        if (bits & WIFI_SCAN_DONE_BIT)
        {
            xEventGroupSetBits(oled_event_group, OLED_SCAN_DONE_BIT);
            wifi_scan_done();
        }
        if (bits & WIFI_CONNECT_START_BIT)
        {
            xEventGroupSetBits(oled_event_group, OLED_CONNECT_START_BIT);
            wifi_disconnect_start();
            vTaskDelay(pdMS_TO_TICKS(1000));
            wifi_connect_start();
        }
        if (bits & WIFI_DISCONNECT_START_BIT)
        {
            xEventGroupSetBits(oled_event_group, OLED_DISCONNECT_START_BIT);
            wifi_disconnect_start();
        }
        if (bits & WIFI_DISCONNECT_DONE_BIT)
        {
            xEventGroupSetBits(oled_event_group, OLED_DISCONNECT_DONE_BIT);
        }
        if (bits & WIFI_CONNECT_DONE_BIT)
        {
            xEventGroupSetBits(oled_event_group, OLED_CONNECT_DONE_BIT);
        }
        if (bits & WIFI_AP_STACONNECTED_BIT)
        {
            xEventGroupSetBits(oled_event_group, OLED_AP_STACONNECTED_BIT);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}