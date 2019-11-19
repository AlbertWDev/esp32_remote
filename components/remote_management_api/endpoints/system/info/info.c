#include "info.h"

static const char* _chip_model_to_str(esp_chip_model_t chip_model)
{
    switch(chip_model) {
        case CHIP_ESP32:
            return "Esp32";
        default:
            return "UNKNOWN";
    }
}

void fill_chip_info(cJSON* node) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    cJSON_AddStringToObject(node, "chip_model", _chip_model_to_str(chip_info.model));
    cJSON_AddNumberToObject(node, "chip_revision", chip_info.revision);
    cJSON_AddNumberToObject(node, "chip_cores", chip_info.cores);
    cJSON_AddBoolToObject(node, "chip_emb_flash", chip_info.features & CHIP_FEATURE_EMB_FLASH);
    cJSON_AddBoolToObject(node, "chip_wifi_bgn", chip_info.features & CHIP_FEATURE_WIFI_BGN);
    cJSON_AddBoolToObject(node, "chip_bt", chip_info.features & CHIP_FEATURE_BT);
    cJSON_AddBoolToObject(node, "chip_ble", chip_info.features & CHIP_FEATURE_BLE);
}

esp_err_t _rmgmt_get_system_info(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    fill_chip_info(root);
    cJSON_AddStringToObject(root, "idf_ver", esp_get_idf_version());
    cJSON_AddNumberToObject(root, "free_heap", esp_get_free_heap_size());

    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);

    free((void *)sys_info);
    cJSON_Delete(root);

    return ESP_OK;
}