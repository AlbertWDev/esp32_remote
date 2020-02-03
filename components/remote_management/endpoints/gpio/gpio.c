#include "gpio.h"

cJSON* _get_gpio_json(uint8_t gpio) {
    if(gpio > 39) return NULL;

    uint8_t out_enabled = ((gpio < 32) ? GPIO.enable : GPIO.enable1.data) & (1 << gpio);

    cJSON* gpio_json = cJSON_CreateObject();
    if(out_enabled) {
        cJSON_AddBoolToObject(gpio_json, "out", ((gpio < 32) ? GPIO.out : GPIO.out1.data) & (1 << gpio)); 
    } else {
        cJSON_AddStringToObject(gpio_json, "out", "disabled");
    }
    cJSON_AddBoolToObject(gpio_json, "in", ((gpio < 32) ? GPIO.in : GPIO.in1.data) & (1 << gpio));
    
    return gpio_json;
}

esp_err_t rest_handler_get_gpio(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");
    cJSON* root = cJSON_CreateObject();

    for(uint8_t gpio = 0; gpio < 40; gpio++) {
        char gpio_num[4];
        sprintf(gpio_num, "%d", gpio);
        cJSON* gpio_json = _get_gpio_json(gpio);
        cJSON_AddItemToObject(root, gpio_num, gpio_json);
    }

    const char *gpio_json = cJSON_Print(root);
    ret = httpd_resp_sendstr(req, gpio_json);

    free((void *)gpio_json);
    cJSON_Delete(root);

    return ret;
}