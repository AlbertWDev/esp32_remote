#pragma once

#include <esp_https_server.h>
#include <esp_err.h>
#include "cJSON.h"
#include "headers.h"

#include <esp_partition.h>
#include <esp_ota_ops.h>

#include <esp_log.h>


esp_err_t _rmgmt_get_system_partitions(httpd_req_t *req);
esp_err_t _rmgmt_get_system_partitions_label(httpd_req_t *req);
esp_err_t _rmgmt_get_system_partitions_label_sha256(httpd_req_t *req);
esp_err_t _rmgmt_put_system_partitions(httpd_req_t *req);
esp_err_t _rmgmt_put_system_partitions_label(httpd_req_t *req);
esp_err_t _rmgmt_put_system_partitions_label_boot(httpd_req_t *req);
esp_err_t _rmgmt_post_system_partitions_label_validate(httpd_req_t *req);
esp_err_t _rmgmt_delete_system_partitions_label(httpd_req_t *req);
