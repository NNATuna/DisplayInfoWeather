# 🌦️ ESP32 Weather Display Project

Dự án ESP32 cấu hình Wi-Fi qua Web UI, lấy dữ liệu thời tiết từ API và hiển thị trên OLED.

## 🧱 Cấu trúc

- `wifi_.*` : Quản lý Wi-Fi scan/connect/disconnect
- `http_server_*` : Web server và giao diện cấu hình
- `http_client_*` : Gọi API thời tiết
- `oled_.*` : Hiển thị thông tin trên OLED

## 🚀 Cách build

```bash
idf.py build
idf.py -p COMx flash monitor
```
