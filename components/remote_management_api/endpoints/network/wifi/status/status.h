#pragma once

#include <esp_https_server.h>
#include <esp_err.h>
#include "cJSON.h"

#include <tcpip_adapter.h>
#include <esp_wifi.h>
#include <lwip/inet.h>


esp_err_t _rmgmt_get_network_wifi_status(httpd_req_t *req);
esp_err_t _rmgmt_put_network_wifi_status(httpd_req_t *req);
