#ifndef HTTP_SERVER_APP_H
#define HTTP_SERVER_APP_H

#include "esp_err.h"
// HTTP request handlers

esp_err_t http_request_webserver_handler(httpd_req_t *req);
esp_err_t http_request_scan_handler(httpd_req_t *req);
esp_err_t http_request_get_scan_handler(httpd_req_t *req);
esp_err_t http_request_connect_handler(httpd_req_t *req);
esp_err_t http_request_disconnect_handler(httpd_req_t *req);
esp_err_t http_request_display_handler(httpd_req_t *req);

#endif // HTTP_SERVER_APP_H