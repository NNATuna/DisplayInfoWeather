#include "http_client_app.h"
#include "lwip/err.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "string.h"
#include "esp_log.h"

static const char *TAG = "HTTP_CLIENT_APP";

char cityName[64] = {0};
weatherData_t weatherData = {0};

static char request_url[256] = {0};

static void cityName_escape_spaces(char *cityNameURL, char *cityName, size_t size)
{
    char temp[128];
    int j = 0;
    for (int i = 0; cityName[i] != '\0' && j < sizeof(temp) - 1; i++)
    {
        if (cityName[i] == ' ')
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
            temp[j++] = cityName[i];
        }
    }
    temp[j] = '\0';
    strlcpy(cityNameURL, temp, size);
}

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        // Có thể log thêm nếu cần
        break;
    case HTTP_EVENT_ON_DATA:
        if (evt->user_data)
        {
            strncat((char *)evt->user_data, (char *)evt->data, evt->data_len);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
    return ESP_OK;
}

esp_err_t http_get_api_weather(weatherData_t *data)
{
    if (strlen(cityName) == 0)
    {
        ESP_LOGW(TAG, "City name is empty, skip weather request");
        return ESP_FAIL;
    }

    char cityNameURL[128];
    cityName_escape_spaces(cityNameURL, cityName, sizeof(cityNameURL));
    snprintf(request_url, sizeof(request_url),
             "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric",
             cityNameURL, OPENWEATHER_API_KEY);

    ESP_LOGI(TAG, "Requesting weather for: %s", cityName);
    ESP_LOGI(TAG, "URL: %s", request_url);

    // Buffer nhận JSON
    char response_buffer[1024] = {0};

    esp_http_client_config_t config = {
        .url = request_url,
        .event_handler = _http_event_handler,
        .user_data = response_buffer,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                 status_code, (int)esp_http_client_get_content_length(client));

        if (status_code == 200)
        {
            // Parse JSON
            cJSON *root = cJSON_Parse(response_buffer);
            if (root)
            {
                cJSON *main = cJSON_GetObjectItem(root, "main");
                cJSON *wind = cJSON_GetObjectItem(root, "wind");

                if (main && wind)
                {
                    cJSON *temp_item = cJSON_GetObjectItem(main, "temp");
                    cJSON *humidity_item = cJSON_GetObjectItem(main, "humidity");
                    cJSON *wind_speed_item = cJSON_GetObjectItem(wind, "speed");

                    if (cJSON_IsNumber(temp_item))
                    {
                        data->temp = temp_item->valuedouble;
                    }
                    if (cJSON_IsNumber(humidity_item))
                    {
                        data->humidity = humidity_item->valueint;
                    }
                    if (cJSON_IsNumber(wind_speed_item))
                    {
                        data->windSpeed = wind_speed_item->valuedouble;
                    }

                    ESP_LOGI(TAG, "Weather updated: %.1f°C, %d%%, %.1fm/s",
                             data->temp, data->humidity, data->windSpeed);
                }
                cJSON_Delete(root);
            }
            else
            {
                ESP_LOGE(TAG, "Failed to parse JSON");
            }
        }
        else
        {
            ESP_LOGE(TAG, "Weather API returned status %d", status_code);
        }
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    esp_http_client_cleanup(client);
    return ESP_OK;
}
