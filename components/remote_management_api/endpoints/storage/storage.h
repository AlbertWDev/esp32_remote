#pragma once

#include <esp_https_server.h>
#include <esp_err.h>
#include "cJSON.h"

#include <esp_vfs.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <esp_log.h>

#define RMGMT_STORAGE_NODE_NAME_MAX_LENGTH 256
#define RMGMT_ERR_STORAGE_NODE_INVALID_NAME 0x80

esp_err_t _rmgmt_get_storage_node(httpd_req_t *req);
esp_err_t _rmgmt_post_storage_node(httpd_req_t *req);
esp_err_t _rmgmt_put_storage_node(httpd_req_t *req);
esp_err_t _rmgmt_delete_storage_node(httpd_req_t *req);
