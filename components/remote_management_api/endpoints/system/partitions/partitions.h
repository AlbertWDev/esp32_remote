#pragma once

#include <esp_https_server.h>
#include <esp_err.h>
#include "cJSON.h"

#include <esp_partition.h>

esp_err_t _rmgmt_get_system_partitions(httpd_req_t *req);
esp_err_t _rmgmt_get_system_partitions_label(httpd_req_t *req);
esp_err_t _rmgmt_put_system_partitions_label(httpd_req_t *req);
esp_err_t _rmgmt_delete_system_partitions_label(httpd_req_t *req);
