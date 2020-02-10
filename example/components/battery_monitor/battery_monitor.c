#include "battery_monitor.h"

battery_event_cb_t _battery_monitor_callback = NULL;

esp_err_t battery_init(battery_event_cb_t callback) {
    _battery_monitor_callback = callback;
    xTaskCreate(batteryMonitorTask, "battery_monitor", 4*1024, NULL, 6, NULL);
    return ESP_OK;
}

// From: https://github.com/juanpabloaj/lolin_d32_pro_with_epd_library/blob/master/deep_sleep/deep_sleep.ino#L76-L92
uint8_t voltageToPercentage(float voltage) {
    return (voltage > 4.19) ? 100
        : voltage <= 3.5 ? 0
        : 2808.3808 * pow(voltage, 4) - 43560.9157 * pow(voltage, 3) + 252848.5888 * pow(voltage, 2) - 650767.4615 * voltage + 626532.5703;
}

void batteryMonitorTask(void *pvParameters)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);

    esp_adc_cal_characteristics_t *adc_chars;
    adc_chars = malloc(sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);

    bat_reading_t reading;

    // Continuously sample ADC1
    while (1) {
        uint32_t adc_reading = 0;
        // Multisampling
        for (int i = 0; i < 10; i++) {
            adc_reading += adc1_get_raw(ADC1_CHANNEL_7);
        }
        adc_reading /= 10;

        // Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

        
        reading.raw = 2*adc_reading;
        reading.voltage = 2*voltage;
        reading.level = voltageToPercentage(2*voltage);
        
        if(_battery_monitor_callback)
            _battery_monitor_callback(&reading);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}