#pragma once

#include <esp_https_server.h>
#include <esp_err.h>
#include "cJSON.h"
#include "headers.h"

#include <esp_system.h>

esp_err_t _rmgmt_get_system_info(httpd_req_t *req);
esp_err_t _rmgmt_get_system_ram(httpd_req_t *req);
esp_err_t _rmgmt_get_system_reboot(httpd_req_t *req);