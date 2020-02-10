#include "info.h"
#include "tcpip_adapter.h"

static const char* _chip_model_to_str(esp_chip_model_t chip_model) {
    switch(chip_model) {
        case CHIP_ESP32:
            return "Esp32";
        default:
            return "UNKNOWN";
    }
}

void _fill_chip_info(cJSON* chip_info_json) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    cJSON_AddStringToObject(chip_info_json, "model", _chip_model_to_str(chip_info.model));
    cJSON_AddNumberToObject(chip_info_json, "revision", chip_info.revision);
    cJSON_AddNumberToObject(chip_info_json, "cores", chip_info.cores);
    cJSON_AddBoolToObject(chip_info_json, "emb_flash", chip_info.features & CHIP_FEATURE_EMB_FLASH);
    cJSON_AddBoolToObject(chip_info_json, "wifi_bgn", chip_info.features & CHIP_FEATURE_WIFI_BGN);
    cJSON_AddBoolToObject(chip_info_json, "bt", chip_info.features & CHIP_FEATURE_BT);
    cJSON_AddBoolToObject(chip_info_json, "ble", chip_info.features & CHIP_FEATURE_BLE);
}

esp_err_t _rmgmt_get_system_info(httpd_req_t *req) {
    ALLOW_CORS(req);
    
    cJSON *sys_info = cJSON_CreateObject();

    // CHIP INFO
    cJSON *chip_info = cJSON_AddObjectToObject(sys_info, "chip");
    _fill_chip_info(chip_info);

    // IDF VERSION
    cJSON_AddStringToObject(sys_info, "idf", esp_get_idf_version());

    // HOSTNAME
    const char* hostname;
    tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &hostname);
    cJSON_AddStringToObject(sys_info, "hostname", hostname);

    // BASE MAC
    uint8_t mac[6];
    if (esp_base_mac_addr_get(mac) != ESP_OK)
        if (esp_efuse_mac_get_custom(mac) != ESP_OK)
            esp_efuse_mac_get_default(mac);
    char* mac_str = (char*) malloc(18);
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(sys_info, "mac", mac_str);

    // TIME
    cJSON_AddNumberToObject(sys_info, "time", esp_timer_get_time());


    const char *sys_info_json = cJSON_Print(sys_info);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, sys_info_json);

    free((void *)sys_info_json);
    cJSON_Delete(sys_info);
    return ESP_OK;
}

esp_err_t _rmgmt_get_system_ram(httpd_req_t *req) {
    ALLOW_CORS(req);

    cJSON* ram = cJSON_CreateObject();
    cJSON_AddNumberToObject(ram, "heap_free", esp_get_free_heap_size());
    cJSON_AddNumberToObject(ram, "heap_total", heap_caps_get_total_size(MALLOC_CAP_DEFAULT));


    const char *ram_json = cJSON_Print(ram);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, ram_json);

    free((void *)ram_json);
    cJSON_Delete(ram);
    return ESP_OK;
}

esp_err_t _rmgmt_get_system_reboot(httpd_req_t *req) {
    ALLOW_CORS(req);
    httpd_resp_send(req, NULL, 0);

    esp_restart();
    return ESP_OK;
}