#include "esp_http_server.h"
#include "http_server_app.h"
#include "http_client_app.h"
#include "cJSON.h"
#include "wifi_app.h"
#include "wifi_task.h"
#include "oled_app.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include <sys/param.h>

static const char *TAG_HTTP_SERVER = "http_server";

#pragma region HTML
static const char resp_html[] =
    "<!DOCTYPE html>\n"
    "<html lang='vi'>\n"
    "<head>\n"
    "  <meta charset='UTF-8' />\n"
    "  <title>Cấu hình Wi-Fi</title>\n"
    "  <script>\n"
    "    function scanWifi() {\n"
    "      alert('Đang quét mạng Wi-Fi...');\n"
    "      fetch('/scan')\n"
    "        .then(response => response.text())\n"
    "        .then(text => {\n"
    "          console.log('Scan response:', text);\n"
    "          alert('Quét xong! Nhấn \"Get Scan\" để tải danh sách.');\n"
    "        })\n"
    "        .catch(err => console.error('Lỗi khi scan:', err));\n"
    "    }\n"
    "\n"
    "    function getScanResult() {\n"
    "      fetch('/get-scan')\n"
    "        .then(response => response.json())\n"
    "        .then(data => {\n"
    "          if (data.status === 'Pending') {\n"
    "            alert('Kết quả scan chưa sẵn sàng. Thử lại sau!');\n"
    "            return;\n"
    "          }\n"
    "          const select = document.getElementById('ssid');\n"
    "          select.innerHTML = '<option value=\"\" disabled selected hidden>-- Chọn mạng Wi-Fi --</option>';\n"
    "          data.forEach((ssid) => {\n"
    "            const option = document.createElement('option');\n"
    "            option.value = ssid;\n"
    "            option.textContent = ssid;\n"
    "            select.appendChild(option);\n"
    "          });\n"
    "          alert('Đã tải danh sách Wi-Fi!');\n"
    "        })\n"
    "        .catch(err => console.error('Lỗi khi lấy danh sách:', err));\n"
    "    }\n"
    "\n"
    "    function connectWifi() {\n"
    "      const ssidSelect = document.getElementById('ssid');\n"
    "      const passInput = document.getElementById('password');\n"
    "      const ssid = ssidSelect ? ssidSelect.value : null;\n"
    "      const pass = passInput ? passInput.value : null;\n"
    "      if (!ssid) {\n"
    "        alert('Chưa chọn Wi-Fi!');\n"
    "        return;\n"
    "      }\n"
    "      alert('Đang kết nối tới ' + ssid + ' ...');\n"
    "      fetch('/connect', {\n"
    "        method: 'POST',\n"
    "        headers: { 'Content-Type': 'application/json' },\n"
    "        body: JSON.stringify({ ssid: ssid, password: pass })\n"
    "      })\n"
    "        .then(res => res.text())\n"
    "        .then(msg => alert(msg))\n"
    "        .catch(err => alert('Lỗi kết nối: ' + err));\n"
    "    }\n"
    "\n"
    "    function displayWeather() {\n"
    "      const cityCode = document.getElementById('cityCode').value;\n"
    "      if (!cityCode) {\n"
    "        alert('City name không hợp lệ');\n"
    "        return;\n"
    "      }\n"
    "      fetch('/display', {\n"
    "        method: 'POST',\n"
    "        headers: { 'Content-Type': 'text/plain' },\n"
    "        body: cityCode\n"
    "      })\n"
    "        .then(res => res.text())\n"
    "        .then(data => console.log('Server trả về:', data))\n"
    "        .catch(err => console.error('Lỗi:', err));\n"
    "    }\n"
    "\n"
    "    function disconnectWifi() {\n"
    "      fetch('/disconnect')\n"
    "        .then(res => res.text())\n"
    "        .then(msg => console.log('Server trả về:', msg))\n"
    "        .catch(err => console.error('Lỗi:', err));\n"
    "    }\n"
    "  </script>\n"
    "</head>\n"
    "<body>\n"
    "  <div class='container'>\n"
    "    <h2>Cấu hình Wi-Fi</h2>\n"
    "    <select id='ssid' name='ssid' required>\n"
    "      <option value='' disabled selected hidden>-- Chọn mạng Wi-Fi --</option>\n"
    "    </select>\n"
    "    <input type='password' id='password' placeholder='Nhập mật khẩu Wi-Fi' />\n"
    "    <button onclick='scanWifi()'>Scan Wi-Fi</button>\n"
    "    <button onclick='getScanResult()'>Get Scan</button>\n"
    "    <button onclick='connectWifi()'>Kết nối</button>\n"
    "    <button onclick='disconnectWifi()'>Ngắt kết nối</button>\n"
    "    <hr />\n"
    "    <input type='text' id='cityCode' placeholder='Nhập City Name' />\n"
    "    <button class='btn btn-display' onclick='displayWeather()'>Display</button>\n"
    "  </div>\n"
    "</body>\n"
    "</html>\n";
#pragma endregion

esp_err_t http_request_webserver_handler(httpd_req_t *req)
{
    httpd_resp_send(req, resp_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
esp_err_t http_request_scan_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG_HTTP_SERVER, "Scan requested by client");
    xEventGroupSetBits(wifi_event_group, WIFI_SCAN_START_BIT);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"Scanning\"}");
    return ESP_OK;
}

esp_err_t http_request_get_scan_handler(httpd_req_t *req)
{

    httpd_resp_set_type(req, "application/json");
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    if (bits & WIFI_GET_SCAN_DONE_BIT)
    {
        httpd_resp_send(req, scan_result_json, strlen(scan_result_json));
        xEventGroupClearBits(wifi_event_group, WIFI_SCAN_DONE_BIT);
    }
    else
    {
        httpd_resp_sendstr(req, "{\"status\":\"Pending\"}");
    }
    return ESP_OK;
}

esp_err_t http_request_connect_handler(httpd_req_t *req)
{
    char content[128];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[recv_size] = '\0';
    ESP_LOGI(TAG_HTTP_SERVER, "Received data: %s", content);

    cJSON *json = cJSON_Parse(content);
    if (json == NULL)
    {
        ESP_LOGE(TAG_HTTP_SERVER, "Failed to parse JSON");
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Failed to parse JSON", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    cJSON *ssid_json = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON *password_json = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (!cJSON_IsString(ssid_json) || (ssid_json->valuestring == NULL) ||
        !cJSON_IsString(password_json) || (password_json->valuestring == NULL))
    {
        ESP_LOGE(TAG_HTTP_SERVER, "Invalid JSON format");
        cJSON_Delete(json);

        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Invalid JSON format\"}", HTTPD_RESP_USE_STRLEN);

        return ESP_FAIL;
    }

    const char *tmp_ssid = ssid_json->valuestring;
    const char *tmp_password = password_json->valuestring;

    ESP_LOGI(TAG_HTTP_SERVER, "Connecting to SSID: %s", ssid);

    // Store credentials in global variables or a secure storage
    strncpy(ssid, tmp_ssid, sizeof(ssid) - 1);
    strncpy(password, tmp_password, sizeof(password) - 1);

    xEventGroupSetBits(wifi_event_group, WIFI_CONNECT_START_BIT);

    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"Connecting\"}");
    return ESP_OK;
}
esp_err_t http_request_disconnect_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG_HTTP_SERVER, "Disconnect requested by client");
    xEventGroupSetBits(wifi_event_group, WIFI_DISCONNECT_START_BIT);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"Disconnecting\"}");
    return ESP_OK;
}
esp_err_t http_request_display_handler(httpd_req_t *req)
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
    strlcpy(cityName, buf, sizeof(cityName));
    ESP_LOGI(TAG_HTTP_SERVER, "Received cityName: %s", cityName);
    xEventGroupSetBits(wifi_event_group, WIFI_GET_WEATHER_START_BIT);
    httpd_resp_sendstr(req, "Data received OK");
    return ESP_OK;
}