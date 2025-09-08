/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "cJSON.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN
/* AP configuration */
#define EXAMPLE_WIFI_AP_SSID CONFIG_ESP_WIFI_AP_SSID
#define EXAMPLE_WIFI_AP_PASS CONFIG_ESP_WIFI_AP_PASSWORD
#define EXAMPLE_WIFI_AP_MAXCONN CONFIG_ESP_WIFI_AP_MAX_CONN
#define EXAMPLE_WIFI_AP_CHANNEL CONFIG_ESP_WIFI_AP_CHANNEL

#define MAX_AP_NUM 20
#define WIFI_CONNECT_DONE_BIT BIT0
#define WIFI_DISCONNECT_DONE_BIT BIT1
#define WIFI_AP_START_BIT BIT2
#define WIFI_SCAN_START_BIT BIT3
#define WIFI_SCAN_DONE_BIT BIT4
#define WIFI_CONNECT_START_BIT BIT5
#define WIFI_AP_STACONNECTED_BIT BIT6
#define WIFI_DISCONNECT_START_BIT BIT7
#define WIFI_GOT_IP_BIT BIT8
#define DISPLAY_WEATHER_BIT BIT9
#define HTTP_START_BIT BIT10
#define WIFI_AP_STADISCONNECTED_BIT BIT11

#define PIN_SDA 21
#define PIN_SCL 22
#define PIN_RST U8G2_ESP32_HAL_UNDEFINED

#define API_KEY_OPENWEATHER "f2aa64b28ac2c1473bfd6e1d72aad511"

static const char *TAG = "http_server_example";
static const char *TAG_AP = "Wifi AP";
static const char *TAG_STA = "WiFi STA";
static char cityCode[64];
static EventGroupHandle_t wifi_event_group;
u8g2_t oledBuf;

typedef enum
{
    OLED_START,
    OLED_INFO_CFG,
    OLED_SCAN_START,
    OLED_SCAN_DONE,
    OLED_CONNECT_START,
    OLED_CONNECT_DONE,
    OLED_DISCONNECT_START,
    OLED_DISCONNECT_DONE,
    OLED_DISPLAY_WEATHER
} oled_id_t;
typedef struct
{
    oled_id_t id;
} oled_msg_t;
typedef struct
{
    double windSpeed; // mô tả thời tiết (vd: "mưa nhẹ")
    double temp;      // nhiệt độ (°C)
    int humidity;     // độ ẩm (%)
} weatherData_t;
char *response_data = NULL;
size_t response_len = 0;
bool all_chunks_received = false;
weatherData_t weatherData;
QueueHandle_t Oled_QueueTask;

#pragma region HTML
static const char resp_html[] =
    "<!DOCTYPE html>\n"
    "<html lang='vi'>\n"
    "<head>\n"
    "    <meta charset='UTF-8' />\n"
    "    <title>Cấu hình Wi-Fi</title>\n"
    "    <script>\n"
    "    function scanWifi() {\n"
    "        alert('Scanning.....');\n"
    "        fetch('/scan')\n"
    "        .then((response) => {\n"
    "            alert('Scan thành công!');\n"
    "            return response.json()\n"
    "        })\n"
    "        .then((data) => {\n"
    "            const select = document.getElementById('ssid');\n"
    "            select.innerHTML = '<option value=\"\" disabled selected hidden>-- Chọn mạng Wi-Fi --</option>';\n"
    "            data.forEach((ssid) => {\n"
    "                const option = document.createElement('option');\n"
    "                option.value = ssid;\n"
    "                option.textContent = ssid;\n"
    "                select.appendChild(option);\n"
    "            });\n"
    "        })\n"
    "        .catch((err) => { console.error('Lỗi:', err); });\n"
    "    }\n"
    "    function connectWifi() {\n"
    "        const ssidSelect = document.getElementById(\"ssid\");\n"
    "        const passInput = document.getElementById(\"password\");\n"
    "        const ssid = ssidSelect ? ssidSelect.value : null;\n"
    "        const pass = passInput ? passInput.value : null;\n"
    "        if (!ssid || !pass) {\n"
    "           alert('Chưa chọn/nhập thông tin wifi!');\n"
    "           return;\n"
    "        }\n"
    "        alert('Connecting....');"
    "        fetch('/connect', {\n"
    "           method: 'POST',\n"
    "           headers: {\n"
    "               'Content-Type': 'application/json',\n"
    "           },\n"
    "           body: JSON.stringify({ ssid: ssid, password: pass }),\n"
    "        })\n"
    "        .then((response) => response.text())\n"
    "        .then((text) => {\n"
    "            alert(text);\n"
    "        })\n"
    "        .catch((error) => {\n"
    "            alert('Lỗi khi kết nối Wi-Fi: ' + error);\n"
    "            console.error(error);\n"
    "        });\n"
    "    }\n"
    "    function displayWeather() {\n"
    "       const inputCD = document.getElementById('cityCode');\n"
    "       const cityCode = inputCD ? inputCD.value : null;\n"
    "       if (!cityCode) {\n"
    "           alert('City name không hợp lệ');\n"
    "           return;\n"
    "           }\n"
    "       fetch('/display', {\n"
    "           method: 'POST',\n"
    "           headers: {\n"
    "               'Content-Type': 'text/plain',\n"
    "           },\n"
    "           body: cityCode,\n"
    "       })\n"
    "       .then(res => res.text())\n"
    "       .then(data => console.log('Server trả về:', data))\n"
    "       .catch(err => console.error('Lỗi:', err));\n"
    "    }\n"
    "    function disconnectWifi() {\n"
    "       fetch('/disconnect')\n"
    "         .then((res) => res.text())\n"
    "         .then((msg) => console.log('Server trả về:', msg))\n"
    "         .catch((err) => console.error('Lỗi:', err));\n"
    "  }\n"
    "    </script>\n"
    "</head>\n"
    "<body>\n"
    "    <div class='container'>\n"
    "        <h2>Cấu hình Wi-Fi</h2>\n"
    "        <select id='ssid' name='ssid' required>\n"
    "            <option value='' disabled selected hidden>-- Chọn mạng Wi-Fi --</option>\n"
    "        </select>\n"
    "        <input type='password' id='password' placeholder='Nhập mật khẩu Wi-Fi' required />\n"
    "        <button onclick='scanWifi()'>Scan</button>\n"
    "        <button onclick='connectWifi()'>Connect</button>\n"
    "        <button onclick='disconnectWifi()'>Disconnect</button>\n"
    "        <input\n"
    "        type='text'\n"
    "        id='cityCode'\n"
    "        placeholder='Nhập City Name'\n"
    "        required\n"
    "        />\n"
    "        <button class='btn btn-display' onclick='displayWeather()'>\n"
    "        Display\n"
    "        </button>\n"
    "    </div>\n"
    "</body>\n"
    "</html>\n";
#pragma endregion

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_START)
    {
        xEventGroupSetBits(wifi_event_group, HTTP_START_BIT);
        xEventGroupSetBits(wifi_event_group, WIFI_AP_START_BIT);
    }
    if (event_id == WIFI_EVENT_SCAN_DONE)
    {
        ESP_LOGI(TAG_AP, "Wifi scan done!");
        xEventGroupSetBits(wifi_event_group, WIFI_SCAN_DONE_BIT);
    }
    if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG_STA, "Wifi connected!");
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECT_DONE_BIT);
    }
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ESP_LOGI(TAG, "GOT IP");
        xEventGroupSetBits(wifi_event_group, WIFI_GOT_IP_BIT);
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        esp_netif_dns_info_t dns;

        if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns) == ESP_OK)
        {
            ESP_LOGI(TAG, "DNS MAIN: " IPSTR, IP2STR(&dns.ip.u_addr.ip4));
        }
        else
        {
            ESP_LOGW(TAG, "No DNS MAIN");
        }

        if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns) == ESP_OK)
        {
            ESP_LOGI(TAG, "DNS BACKUP: " IPSTR, IP2STR(&dns.ip.u_addr.ip4));
        }
    }
    if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_DISCONNECT_DONE_BIT);
    }
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG_AP, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
        xEventGroupSetBits(wifi_event_group, WIFI_AP_STACONNECTED_BIT);
    }
    if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        xEventGroupSetBits(wifi_event_group, WIFI_AP_STACONNECTED_BIT);
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG_AP, "station " MACSTR " leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

esp_err_t Wifi_Init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(ap_netif != NULL);
    assert(sta_netif != NULL);

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

    wifi_config_t ap_config = {
        .ap = {
            .ssid = EXAMPLE_WIFI_AP_SSID,
            .ssid_len = strlen(EXAMPLE_WIFI_AP_SSID),
            .channel = 1,
            .password = EXAMPLE_WIFI_AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen((char *)ap_config.ap.password) == 0)
    {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config_t sta_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("Wifi_Init", "Wi-Fi init done. AP: %s, Password: %s",
             ap_config.ap.ssid, ap_config.ap.password);

    return ESP_OK;
}
esp_err_t Weather_Data_Json_Prase(const char *json)
{
    cJSON *root = cJSON_Parse(json);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    cJSON *main = cJSON_GetObjectItem(root, "main");
    if (cJSON_IsObject(main))
    {
        cJSON *temp = cJSON_GetObjectItem(main, "temp");
        cJSON *humidity = cJSON_GetObjectItem(main, "humidity");
        if (cJSON_IsNumber(temp))
        {
            weatherData.temp = temp->valuedouble;
        }
        if (cJSON_IsNumber(humidity))
        {
            weatherData.humidity = humidity->valueint;
        }
    }

    cJSON *wind = cJSON_GetObjectItem(root, "wind");
    if (cJSON_IsObject(wind))
    {
        cJSON *speed = cJSON_GetObjectItem(wind, "speed");
        if (cJSON_IsNumber(speed))
        {
            weatherData.windSpeed = speed->valuedouble;
        }
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "Parsed Weather Data: Temp=%.2f°C, Humidity=%d%%, Wind Speed=%.2fm/s",
             weatherData.temp, weatherData.humidity, weatherData.windSpeed);

    xQueueSend(Oled_QueueTask, &(oled_msg_t){.id = OLED_DISPLAY_WEATHER}, portMAX_DELAY);

    return ESP_OK;
}
void cityCode_escape_spaces(char *cityCode, size_t size)
{
    char temp[128];
    int j = 0;
    for (int i = 0; cityCode[i] != '\0' && j < sizeof(temp) - 1; i++)
    {
        if (cityCode[i] == ' ')
        {
            if (j + 3 < sizeof(temp) - 1) // còn chỗ để thêm "%20"
            {
                temp[j++] = '%';
                temp[j++] = '2';
                temp[j++] = '0';
            }
        }
        else
        {
            temp[j++] = cityCode[i];
        }
    }
    temp[j] = '\0';
    strlcpy(cityCode, temp, size);
}

esp_err_t scan_handler(httpd_req_t *req)
{
    xEventGroupSetBits(wifi_event_group, WIFI_SCAN_START_BIT);
    uint16_t ap_count = 0;
    wifi_ap_record_t ap_info[MAX_AP_NUM];
    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true};
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_cfg, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    if (ap_count > MAX_AP_NUM)
        ap_count = MAX_AP_NUM;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));

    char json[1024];
    strcpy(json, "[");
    for (int i = 0; i < ap_count; i++)
    {
        char buf[64];
        sprintf(buf, "\"%s\"%s", ap_info[i].ssid, (i < ap_count - 1) ? "," : "");
        strcat(json, buf);
    }
    strcat(json, "]");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));
    return ESP_OK;
}
esp_err_t disconnect_handler(httpd_req_t *req)
{
    xEventGroupSetBits(wifi_event_group, WIFI_DISCONNECT_START_BIT);
    if (esp_wifi_disconnect() == ESP_OK)
        httpd_resp_send(req, NULL, 0);
    else
        httpd_resp_send_500(req);
    return ESP_OK;
}
esp_err_t connect_handler(httpd_req_t *req)
{
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECT_START_BIT);
    int total_len = req->content_len;
    if (total_len <= 0)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    char *buffer = malloc(total_len + 1);
    if (!buffer)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    int received = 0;
    while (received < total_len)
    {
        int ret = httpd_req_recv(req, buffer + received, total_len - received);
        if (ret <= 0)
        {
            free(buffer);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        received += ret;
    }
    buffer[total_len] = '\0';
    cJSON *json = cJSON_Parse(buffer);

    cJSON *ssid_item = cJSON_GetObjectItem(json, "ssid");
    cJSON *password_item = cJSON_GetObjectItem(json, "password");

    if (ssid_item && cJSON_IsString(ssid_item) && ssid_item->valuestring != NULL)
    {
        const char *ssid_s = ssid_item->valuestring;
        const char *password_s = password_item->valuestring;

        wifi_config_t wifi_cfg = {0};

        strncpy((char *)wifi_cfg.sta.ssid, ssid_s, sizeof(wifi_cfg.sta.ssid) - 1);
        wifi_cfg.sta.ssid[sizeof(wifi_cfg.sta.ssid) - 1] = '\0';

        strncpy((char *)wifi_cfg.sta.password, password_s, sizeof(wifi_cfg.sta.password) - 1);
        wifi_cfg.sta.password[sizeof(wifi_cfg.sta.password) - 1] = '\0';

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));

        if (esp_wifi_connect() == ESP_OK)
            httpd_resp_sendstr(req, "WiFi connected successfully!");
        else
            httpd_resp_sendstr(req, "WiFi connected failed!");
    }
    return ESP_OK;
}
esp_err_t display_handler(httpd_req_t *req)
{
    char buf[128];
    int total_len = req->content_len;
    int received = 0;

    while (received < total_len)
    {
        int r = httpd_req_recv(req, buf + received, total_len - received);
        if (r <= 0)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read body");
            return ESP_FAIL;
        }
        received += r;
    }

    if (received >= sizeof(buf))
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too big");
        return ESP_FAIL;
    }

    buf[received] = '\0';
    strlcpy(cityCode, buf, sizeof(cityCode));
    ESP_LOGI(TAG, "Received cityCode: %s", cityCode);
    cityCode_escape_spaces(cityCode, sizeof(cityCode));
    xEventGroupSetBits(wifi_event_group, DISPLAY_WEATHER_BIT);
    httpd_resp_sendstr(req, "Data received OK");
    return ESP_OK;
}
esp_err_t webserver_handler(httpd_req_t *req)
{
    httpd_resp_send(req, resp_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
esp_err_t start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    httpd_handle_t server = NULL;
    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_uri_t test_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = webserver_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &test_uri);
        httpd_uri_t scan_uri = {
            .uri = "/scan",
            .method = HTTP_GET,
            .handler = scan_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &scan_uri);
        httpd_uri_t connect_uri = {
            .uri = "/connect",
            .method = HTTP_POST,
            .handler = connect_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &connect_uri);
        httpd_uri_t display_uri = {
            .uri = "/display",
            .method = HTTP_POST,
            .handler = display_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &display_uri);
        httpd_uri_t disconnect_uri = {
            .uri = "/disconnect",
            .method = HTTP_GET,
            .handler = disconnect_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &disconnect_uri);
    }
    return ESP_OK;
}
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        response_data = realloc(response_data, response_len + evt->data_len);
        memcpy(response_data + response_len, evt->data, evt->data_len);
        response_len += evt->data_len;
        break;
    case HTTP_EVENT_ON_FINISH:
        all_chunks_received = true;
        ESP_LOGI("OpenWeatherAPI", "Received data: %s", response_data);
        Weather_Data_Json_Prase(response_data);
        break;
    default:
        break;
    }
    return ESP_OK;
}

esp_err_t http_get_api_weather(void)
{
    if (strlen(cityCode) == 0)
    {
        ESP_LOGE(TAG, "City code is empty");
        return ESP_FAIL;
    }

    char url[256];
    snprintf(url, sizeof(url),
             "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric&lang=vi",
             cityCode, API_KEY_OPENWEATHER);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200)
        {
            ESP_LOGI(TAG, "Message sent Successfully");
        }
        else
        {
            ESP_LOGI(TAG, "Message sent Failed");
        }
    }
    else
    {
        ESP_LOGI(TAG, "Message sent Failed");
    }
    esp_http_client_cleanup(client);
    return ESP_OK;
}

esp_err_t Oled_Init(void)
{
    u8g2_esp32_hal_t hal = U8G2_ESP32_HAL_DEFAULT;
    hal.bus.i2c.sda = PIN_SDA;
    hal.bus.i2c.scl = PIN_SCL;
    hal.reset = PIN_RST;
    u8g2_esp32_hal_init(hal);

    u8g2_Setup_sh1106_i2c_128x64_noname_f(
        &oledBuf,
        U8G2_R0,
        u8g2_esp32_i2c_byte_cb,
        u8g2_esp32_gpio_and_delay_cb);
    u8x8_SetI2CAddress(&oledBuf.u8x8, 0x3C << 1);
    u8g2_InitDisplay(&oledBuf);
    u8g2_SetPowerSave(&oledBuf, 0);

    u8g2_ClearBuffer(&oledBuf);

    u8g2_SetFont(&oledBuf, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "Xin chao!");
    u8g2_SendBuffer(&oledBuf);

    return ESP_OK;
}
esp_err_t Draw_Oled_Start(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "Wifi AP start: ");
    u8g2_SetFont(&oledBuf, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&oledBuf, 0, 40, "Ssid: " EXAMPLE_WIFI_AP_SSID);
    u8g2_DrawStr(&oledBuf, 0, 60, "Password: " EXAMPLE_WIFI_AP_PASS);
    u8g2_SendBuffer(&oledBuf);
    return ESP_OK;
}
esp_err_t Draw_Oled_Info_Cfg(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "Connect to IP: ");
    u8g2_DrawStr(&oledBuf, 0, 40, "192.168.4.1");
    u8g2_DrawStr(&oledBuf, 0, 60, "To config Wifi");
    u8g2_SendBuffer(&oledBuf);
    return ESP_OK;
}
esp_err_t Draw_Oled_Scan_Start(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "SCANNING.....");
    u8g2_SendBuffer(&oledBuf);
    return ESP_OK;
}
esp_err_t Draw_Oled_Scan_Done(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "SCAN DONE!");
    u8g2_SendBuffer(&oledBuf);
    return ESP_OK;
}
esp_err_t Draw_Oled_Connect_Start(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "Connecting....");
    u8g2_SendBuffer(&oledBuf);
    return ESP_OK;
}
esp_err_t Draw_Oled_Connect_Done(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "Connected successfully");
    u8g2_SendBuffer(&oledBuf);
    return ESP_OK;
}
esp_err_t Draw_Oled_Disconnect_Start(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "Disconnecting....");
    u8g2_SendBuffer(&oledBuf);
    return ESP_OK;
}
esp_err_t Draw_Oled_Disconnect_Done(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "Disconnected.");
    u8g2_SendBuffer(&oledBuf);
    return ESP_OK;
}
esp_err_t Draw_Oled_Display(void)
{
    http_get_api_weather();
    char line1[32];
    char line2[32];
    char line3[64];

    snprintf(line1, sizeof(line1), "Nhiet do: %.1fC", weatherData.temp);
    snprintf(line2, sizeof(line2), "Do am: %d%%", weatherData.humidity);
    snprintf(line3, sizeof(line3), "Gio: %.1f m/s", weatherData.windSpeed);

    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);

    u8g2_DrawStr(&oledBuf, 0, 15, line1);
    u8g2_DrawStr(&oledBuf, 0, 30, line2);
    u8g2_DrawStr(&oledBuf, 0, 45, line3);

    u8g2_SendBuffer(&oledBuf);

    return ESP_OK;
}

void event_task(void *pvParameters)
{
    EventBits_t bits;
    oled_msg_t msg;
    while (1)
    {
        bits = xEventGroupWaitBits(wifi_event_group,
                                   WIFI_CONNECT_DONE_BIT |
                                       WIFI_DISCONNECT_DONE_BIT |
                                       WIFI_AP_START_BIT |
                                       WIFI_SCAN_START_BIT |
                                       WIFI_SCAN_DONE_BIT |
                                       WIFI_CONNECT_START_BIT |
                                       WIFI_AP_STACONNECTED_BIT |
                                       WIFI_DISCONNECT_START_BIT |
                                       WIFI_GOT_IP_BIT,
                                   pdTRUE, pdFALSE, portMAX_DELAY);
        if (bits & WIFI_CONNECT_DONE_BIT)
        {
            msg.id = OLED_CONNECT_DONE;
            xQueueSend(Oled_QueueTask, &msg, pdMS_TO_TICKS(100));
        }
        if (bits & WIFI_AP_STADISCONNECTED_BIT)
        {
            msg.id = OLED_START;
            xQueueSend(Oled_QueueTask, &msg, pdMS_TO_TICKS(100));
        }
        if (bits & WIFI_AP_STACONNECTED_BIT)
        {
            msg.id = OLED_INFO_CFG;
            xQueueSend(Oled_QueueTask, &msg, pdMS_TO_TICKS(100));
        }
        if (bits & WIFI_DISCONNECT_DONE_BIT)
        {
            msg.id = OLED_DISCONNECT_DONE;
            xQueueSend(Oled_QueueTask, &msg, pdMS_TO_TICKS(100));
        }
        if (bits & WIFI_AP_START_BIT)
        {
            msg.id = OLED_START;
            xQueueSend(Oled_QueueTask, &msg, pdMS_TO_TICKS(100));
        }
        if (bits & WIFI_SCAN_START_BIT)
        {
            msg.id = OLED_SCAN_START;
            xQueueSend(Oled_QueueTask, &msg, pdMS_TO_TICKS(100));
        }
        if (bits & WIFI_SCAN_DONE_BIT)
        {
            msg.id = OLED_SCAN_DONE;
            xQueueSend(Oled_QueueTask, &msg, pdMS_TO_TICKS(100));
        }
        if (bits & WIFI_CONNECT_START_BIT)
        {
            msg.id = OLED_CONNECT_START;
            xQueueSend(Oled_QueueTask, &msg, pdMS_TO_TICKS(100));
        }
        if (bits & WIFI_DISCONNECT_START_BIT)
        {
            msg.id = OLED_DISCONNECT_START;
            xQueueSend(Oled_QueueTask, &msg, pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void oled_task(void *pvParameters)
{
    ESP_ERROR_CHECK(Oled_Init());
    oled_msg_t msg;
    while (1)
    {
        if (xQueueReceive(Oled_QueueTask, &msg, portMAX_DELAY) == pdPASS)
        {
            switch (msg.id)
            {
            case OLED_START:
                Draw_Oled_Start();
                break;
            case OLED_INFO_CFG:
                Draw_Oled_Info_Cfg();
                break;
            case OLED_SCAN_START:
                Draw_Oled_Scan_Start();
                break;
            case OLED_SCAN_DONE:
                Draw_Oled_Scan_Done();
                break;
            case OLED_CONNECT_START:
                Draw_Oled_Connect_Start();
                break;
            case OLED_CONNECT_DONE:
                Draw_Oled_Connect_Done();
                break;
            case OLED_DISCONNECT_START:
                Draw_Oled_Disconnect_Start();
                break;
            case OLED_DISCONNECT_DONE:
                Draw_Oled_Disconnect_Done();
                break;
            case OLED_DISPLAY_WEATHER:
                Draw_Oled_Display();
                break;
            default:
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void Timer_API_Weather_Task(void *pvParameters)
{
    oled_msg_t msg;
    msg.id = OLED_DISPLAY_WEATHER;

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, DISPLAY_WEATHER_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    http_get_api_weather();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10 * 60 * 1000));
        if (response_data != NULL)
        {
            free(response_data);
            response_data = NULL;
            response_len = 0;
            all_chunks_received = false;
        }
        bits = xEventGroupGetBits(wifi_event_group);
        if (bits & DISPLAY_WEATHER_BIT)
        {
            http_get_api_weather();
        }
    }
}
void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_event_group = xEventGroupCreate();
    Oled_QueueTask = xQueueCreate(10, sizeof(oled_msg_t));
    xTaskCreate(oled_task, "oled_task", 5120, NULL, 5, NULL);
    xTaskCreate(event_task, "event_task", 4096, NULL, 5, NULL);
    xTaskCreate(Timer_API_Weather_Task, "Timer_API_Weather_Task", 4096, NULL, 5, NULL);

    ESP_ERROR_CHECK(Wifi_Init());

    xEventGroupWaitBits(wifi_event_group, HTTP_START_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    ESP_ERROR_CHECK(start_http_server());
}
