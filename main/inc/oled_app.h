#ifndef OLED_APP_H
#define OLED_APP_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_bit_defs.h"

#define OLED_START_BIT BIT0
#define OLED_SCAN_START_BIT BIT1
#define OLED_SCAN_DONE_BIT BIT2
#define OLED_CONNECT_START_BIT BIT3
#define OLED_CONNECT_DONE_BIT BIT4
#define OLED_DISCONNECT_START_BIT BIT5
#define OLED_DISCONNECT_DONE_BIT BIT6
#define OLED_GOT_IP_BIT BIT7
#define OLED_WEATHER_START_BIT BIT8
#define OLED_AP_STACONNECTED_BIT BIT9

extern EventGroupHandle_t oled_event_group;

void Oled_Init(void);
void Draw_Oled_Start(void);
void Draw_Oled_Info_Cfg(void);
void Draw_Oled_Scan_Start(void);
void Draw_Oled_Scan_Done(void);
void Draw_Oled_Connect_Start(void);
void Draw_Oled_Connect_Done(void);
void Draw_Oled_Disconnect_Start(void);
void Draw_Oled_Disconnect_Done(void);
void Draw_Oled_Display(void);
#endif // OLED_APP_H