#include "oled_app.h"
#include "http_client_app.h"
#include "u8g2.h"
#include "u8g2_esp32_hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define PIN_SDA 21
#define PIN_SCL 22
#define PIN_RST U8G2_ESP32_HAL_UNDEFINED

static u8g2_t oledBuf;
EventGroupHandle_t oled_event_group;

void Oled_Init(void)
{
    oled_event_group = xEventGroupCreate();
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
}
void Draw_Oled_Start(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "Wifi AP start: ");
    u8g2_SetFont(&oledBuf, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&oledBuf, 0, 40, "Ssid: " CONFIG_ESP_WIFI_AP_SSID);
    u8g2_DrawStr(&oledBuf, 0, 60, "Password: " CONFIG_ESP_WIFI_AP_PASSWORD);
    u8g2_SendBuffer(&oledBuf);
}
void Draw_Oled_Info_Cfg(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "Connect to IP: ");
    u8g2_DrawStr(&oledBuf, 0, 40, "192.168.4.1");
    u8g2_DrawStr(&oledBuf, 0, 60, "To config Wifi");
    u8g2_SendBuffer(&oledBuf);
}
void Draw_Oled_Scan_Start(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "SCANNING.....");
    u8g2_SendBuffer(&oledBuf);
}
void Draw_Oled_Scan_Done(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "SCAN DONE!");
    u8g2_SendBuffer(&oledBuf);
}
void Draw_Oled_Connect_Start(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "Connecting....");
    u8g2_SendBuffer(&oledBuf);
}
void Draw_Oled_Connect_Done(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "Connected successfully");
    u8g2_SendBuffer(&oledBuf);
}
void Draw_Oled_Disconnect_Start(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "Disconnecting....");
    u8g2_SendBuffer(&oledBuf);
}
void Draw_Oled_Disconnect_Done(void)
{
    u8g2_ClearBuffer(&oledBuf);
    u8g2_SetFont(&oledBuf, u8g2_font_6x10_tr);
    u8g2_DrawStr(&oledBuf, 0, 20, "CONFIG WIFI");
    u8g2_DrawStr(&oledBuf, 0, 40, "Disconnected.");
    u8g2_SendBuffer(&oledBuf);
}
void Draw_Oled_Display(void)
{
    char line1[64];
    char line2[64];
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
}
