#include "wifi_task.h"
#include "http_server_task.h"
#include "oled_task.h"
#include "http_client_task.h"

void app_main(void)
{
    xTaskCreate(&wifi_task, "wifi_task", 4096, NULL, 5, NULL);
    xTaskCreate(&http_server_task, "http_server_task", 8192, NULL, 4, NULL);
    xTaskCreate(&oled_task, "oled_task", 4096, NULL, 3, NULL);
    xTaskCreate(&http_client_task, "http_client_task", 4096, NULL, 3, NULL);
}
