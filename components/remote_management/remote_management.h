#pragma once

#include <esp_err.h>
#include <esp_event.h>
#include <esp_https_server.h>
#include <esp_log.h>

#include "wifi_manager.h"


extern const size_t _rmgmt_endpoints_len;
extern const httpd_uri_t _rmgmt_endpoints[];



typedef struct rmgmt_server_t {
    httpd_handle_t server_handle;
    httpd_uri_t* endpoints;
    size_t endpoints_len;
    ssl_certs_t* ssl_certs;
} rmgmt_server_t;


/*
 * Initialize the Remote Management API
 */
esp_err_t rmgmt_init(ssl_certs_t* ssl_certs, const httpd_uri_t* user_endpoints, size_t user_endpoints_len);
void rmgmt_release();

esp_err_t rmgmt_start(rmgmt_server_t* rmgmt_server);
void rmgmt_stop(httpd_handle_t* server_handle);