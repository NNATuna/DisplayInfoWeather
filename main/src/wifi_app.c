#include "wifi_app.h"
#include "oled_app.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG_WIFI = "wifi_app";
char scan_result_json[1024];
char ssid[32];
char password[64];
uint16_t ap_count;
wifi_ap_record_t ap_info[MAX_AP_SCAN];
EventGroupHandle_t wifi_event_group;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG_WIFI, "WiFi started, connecting to AP...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_SCAN_DONE_BIT);
        ESP_LOGI(TAG_WIFI, "WiFi scan done.");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_DISCONNECT_DONE_BIT);
        ESP_LOGI(TAG_WIFI, "WiFi disconnected, trying to reconnect...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG_WIFI, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        xEventGroupSetBits(wifi_event_group, WIFI_AP_STACONNECTED_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG_WIFI, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECT_DONE_BIT);
    }
    else if (event_id == WIFI_EVENT_AP_START)
    {
        ESP_LOGI(TAG_WIFI, "WiFi AP started. SSID:%s password:%s",
                 CONFIG_ESP_WIFI_AP_SSID, CONFIG_ESP_WIFI_AP_PASSWORD);
        xEventGroupSetBits(wifi_event_group, WIFI_AP_START_BIT);
    }
}

void wifi_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
}

void wifi_init_apsta(void)
{
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_config_t ap_config = {
        .ap = {
            .ssid = CONFIG_ESP_WIFI_AP_SSID,
            .ssid_len = strlen(CONFIG_ESP_WIFI_AP_SSID),
            .channel = CONFIG_ESP_WIFI_CHANNEL,
            .password = CONFIG_ESP_WIFI_AP_PASSWORD,
            .max_connection = CONFIG_ESP_WIFI_AP_MAX_CONNECT,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen(CONFIG_ESP_WIFI_AP_PASSWORD) == 0)
    {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "AP started: SSID:%s password:%s",
             CONFIG_ESP_WIFI_AP_SSID, CONFIG_ESP_WIFI_AP_PASSWORD);
}

void wifi_scan_start(void)
{
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true};
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
}
void wifi_scan_done(void)
{
    ESP_LOGI(TAG_WIFI, "Getting WiFi scan results...");
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    if (ap_count > MAX_AP_SCAN)
        ap_count = MAX_AP_SCAN;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));
    strcpy(scan_result_json, "[");
    for (int i = 0; i < ap_count; i++)
    {
        strcat(scan_result_json, "\"");
        strcat(scan_result_json, (char *)ap_info[i].ssid);
        strcat(scan_result_json, "\"");
        if (i < ap_count - 1)
            strcat(scan_result_json, ",");
    }
    strcat(scan_result_json, "]");

    ESP_LOGI(TAG_WIFI, "Scan JSON: %s", scan_result_json);
    xEventGroupSetBits(wifi_event_group, WIFI_GET_SCAN_DONE_BIT);
}
void wifi_connect_start(void)
{
    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    wifi_cfg.sta.ssid[sizeof(wifi_cfg.sta.ssid) - 1] = '\0';
    if (strlen(password) > 0)
    {
        strncpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password) - 1);
        wifi_cfg.sta.password[sizeof(wifi_cfg.sta.password) - 1] = '\0';
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_LOGI(TAG_WIFI, "Connecting to SSID:%s password:%s", ssid, password);
    ESP_ERROR_CHECK(esp_wifi_connect());
}
void wifi_disconnect_start(void)
{
    ESP_LOGI(TAG_WIFI, "Disconnecting from AP...");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
}