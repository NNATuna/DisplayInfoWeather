#ifndef WIFI_APP_H
#define WIFI_APP_H

#include "esp_bit_defs.h"
#include "esp_wifi.h"
#define WIFI_AP_START_BIT BIT0
#define WIFI_SCAN_START_BIT BIT1
#define WIFI_SCAN_DONE_BIT BIT2
#define WIFI_CONNECT_START_BIT BIT3
#define WIFI_CONNECT_DONE_BIT BIT4
#define WIFI_DISCONNECT_START_BIT BIT5
#define WIFI_DISCONNECT_DONE_BIT BIT6
#define WIFI_AP_STACONNECTED_BIT BIT8
#define WIFI_GET_WEATHER_START_BIT BIT9
#define WIFI_GET_SCAN_DONE_BIT BIT10
#define WIFI_GET_WEATHER_CLK_BIT BIT11
#define MAX_AP_SCAN 20

extern EventGroupHandle_t wifi_event_group;
extern char scan_result_json[1024];
extern char ssid[32];
extern char password[64];
extern uint16_t ap_count;
extern wifi_ap_record_t ap_info[MAX_AP_SCAN];

void wifi_init(void);
void wifi_init_apsta(void);
void wifi_scan_start(void);
void wifi_scan_done(void);
void wifi_connect_start(void);
void wifi_disconnect_start(void);

#endif