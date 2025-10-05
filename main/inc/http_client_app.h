#ifndef HTTP_CLIENT_APP_H
#define HTTP_CLIENT_APP_H

#include "esp_err.h"
#define OPENWEATHER_API_KEY "f2aa64b28ac2c1473bfd6e1d72aad511"

extern char cityName[64];
typedef struct
{
    double windSpeed;
    double temp;
    int humidity;
} weatherData_t;
extern weatherData_t weatherData;

esp_err_t http_get_api_weather(weatherData_t *data);

#endif // HTTP_CLIENT_APP_H