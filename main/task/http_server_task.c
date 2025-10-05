#include "http_server_task.h"
#include "esp_http_server.h"
#include "http_server_app.h"
#include "wifi_app.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "http_server_task";

void http_server_task(void *pvParameters)
{
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_AP_START_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting HTTP Server");

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t web_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = http_request_webserver_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &web_uri);

        httpd_uri_t scan_uri = {
            .uri = "/scan",
            .method = HTTP_GET,
            .handler = http_request_scan_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &scan_uri);

        httpd_uri_t get_scan_uri = {
            .uri = "/get-scan",
            .method = HTTP_GET,
            .handler = http_request_get_scan_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &get_scan_uri);

        httpd_uri_t connect_uri = {
            .uri = "/connect",
            .method = HTTP_POST,
            .handler = http_request_connect_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &connect_uri);

        httpd_uri_t display_uri = {
            .uri = "/display",
            .method = HTTP_POST,
            .handler = http_request_display_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &display_uri);

        httpd_uri_t disconnect_uri = {
            .uri = "/disconnect",
            .method = HTTP_GET,
            .handler = http_request_disconnect_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &disconnect_uri);
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
