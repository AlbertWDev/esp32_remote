#pragma once

#include <esp_https_server.h>
#include <esp_err.h>
#include "cJSON.h"

#include <soc/gpio_struct.h>
#include <driver/gpio.h>

esp_err_t rest_handler_get_gpio(httpd_req_t *req);