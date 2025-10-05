#include "esp_err.h"
#include "oled_app.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

void oled_task(void *pvParameters)
{
    Oled_Init();
    Draw_Oled_Start();

    while (1)
    {
        EventBits_t bits = xEventGroupWaitBits(oled_event_group,
                                               OLED_START_BIT | OLED_SCAN_START_BIT | OLED_SCAN_DONE_BIT |
                                                   OLED_CONNECT_START_BIT | OLED_CONNECT_DONE_BIT |
                                                   OLED_DISCONNECT_START_BIT | OLED_DISCONNECT_DONE_BIT |
                                                   OLED_GOT_IP_BIT | OLED_WEATHER_START_BIT | OLED_AP_STACONNECTED_BIT,
                                               pdTRUE,
                                               pdFALSE,
                                               portMAX_DELAY);
        if (bits & OLED_SCAN_START_BIT)
        {
            Draw_Oled_Scan_Start();
        }
        if (bits & OLED_SCAN_DONE_BIT)
        {
            Draw_Oled_Scan_Done();
        }
        if (bits & OLED_CONNECT_START_BIT)
        {
            Draw_Oled_Connect_Start();
        }
        if (bits & OLED_CONNECT_DONE_BIT)
        {
            Draw_Oled_Connect_Done();
        }
        if (bits & OLED_DISCONNECT_START_BIT)
        {
            Draw_Oled_Disconnect_Start();
        }
        if (bits & OLED_DISCONNECT_DONE_BIT)
        {
            Draw_Oled_Disconnect_Done();
        }
        if (bits & OLED_AP_STACONNECTED_BIT)
        {
            Draw_Oled_Info_Cfg();
        }
        if (bits & OLED_WEATHER_START_BIT)
        {
            Draw_Oled_Display();
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}