#pragma once

#include <esp_https_server.h>
#include <esp_err.h>
#include "cJSON.h"

#include "wifi_manager.h"


#define RMGMT_ERR_NETWORK_WIFI_SSID_INVALID             0x500
#define RMGMT_ERR_NETWORK_WIFI_SSID_REQUIRED            0x501
#define RMGMT_ERR_NETWORK_WIFI_REQ_CONTENT_TOO_LONG     0x502
#define RMGMT_ERR_NETWORK_WIFI_REQ_CONTENT_NOT_RECEIVED 0x503
#define RMGMT_ERR_NETWORK_WIFI_REQ_CONTENT_INVALID      0x504



esp_err_t _rmgmt_get_network_wifi_storage(httpd_req_t *req);
esp_err_t _rmgmt_get_network_wifi_storage_ssid(httpd_req_t *req);
esp_err_t _rmgmt_post_network_wifi_storage(httpd_req_t *req);
esp_err_t _rmgmt_put_network_wifi_storage_ssid(httpd_req_t *req);
esp_err_t _rmgmt_delete_network_wifi_storage_ssid(httpd_req_t *req);