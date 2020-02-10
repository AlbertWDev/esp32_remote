#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_log.h"

#include <driver/adc.h>
#include "esp_adc_cal.h"
#include "math.h"

#define DEFAULT_VREF    1103    //Use adc2_vref_to_gpio() to obtain a better estimate

typedef struct bat_reading_t {
    uint32_t raw;
    uint32_t voltage;
    uint8_t level;
} bat_reading_t;

typedef void (*battery_event_cb_t)(bat_reading_t* battery_reading);
battery_event_cb_t _battery_monitor_callback;

esp_err_t battery_init(battery_event_cb_t callback);

void batteryMonitorTask(void*);