#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include "wifi_app.h"
#define MAX_AP_SCAN 20

extern char scan_result_json[1024];
extern char ssid[32];
extern char password[64];

void wifi_task(void *pvParameters);

#endif // WIFI_TASK_H