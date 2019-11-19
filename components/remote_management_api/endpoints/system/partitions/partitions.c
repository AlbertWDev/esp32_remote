#include "partitions.h"

#define PARTITION_LABEL_MAX_LENGTH 16
#define REST_ERR_STORAGE_PARTITION_INVALID_LABEL 0x70

esp_err_t _get_partition_label_from_uri(const char* uri, char* label, size_t max_len) {
    size_t uri_len = strlen(uri);
    size_t offset = strlen("/v1/system/partitions/");

    if(offset >= uri_len)
        return REST_ERR_STORAGE_PARTITION_INVALID_LABEL;
    
    strncpy(label, uri + offset, max_len - 1);
    label[max_len - 1] = '\0';
    return ESP_OK;
}

static const char* _partition_type_to_str(esp_partition_type_t type)
{
    switch(type) {
        case ESP_PARTITION_TYPE_APP:
            return "APP";
        case ESP_PARTITION_TYPE_DATA:
            return "DATA";
        default:
            return "UNKNOWN";
    }
}

static const char* _partition_subtype_to_str(esp_partition_type_t type, esp_partition_subtype_t subtype)
{
    if(type == ESP_PARTITION_TYPE_APP)
        switch(subtype) {
            case ESP_PARTITION_SUBTYPE_APP_FACTORY:
                return "FACTORY";
            case ESP_PARTITION_SUBTYPE_APP_OTA_0:
                return "OTA_0";
            case ESP_PARTITION_SUBTYPE_APP_OTA_1:
                return "OTA_1";
            case ESP_PARTITION_SUBTYPE_APP_OTA_2:
                return "OTA_2";
            case ESP_PARTITION_SUBTYPE_APP_OTA_3:
                return "OTA_3";
            case ESP_PARTITION_SUBTYPE_APP_OTA_4:
                return "OTA_4";
            case ESP_PARTITION_SUBTYPE_APP_OTA_5:
                return "OTA_5";
            case ESP_PARTITION_SUBTYPE_APP_OTA_6:
                return "OTA_6";
            case ESP_PARTITION_SUBTYPE_APP_OTA_7:
                return "OTA_7";
            case ESP_PARTITION_SUBTYPE_APP_OTA_8:
                return "OTA_8";
            case ESP_PARTITION_SUBTYPE_APP_OTA_9:
                return "OTA_9";
            case ESP_PARTITION_SUBTYPE_APP_OTA_10:
                return "OTA_10";
            case ESP_PARTITION_SUBTYPE_APP_OTA_11:
                return "OTA_11";
            case ESP_PARTITION_SUBTYPE_APP_OTA_12:
                return "OTA_12";
            case ESP_PARTITION_SUBTYPE_APP_OTA_13:
                return "OTA_13";
            case ESP_PARTITION_SUBTYPE_APP_OTA_14:
                return "OTA_14";
            case ESP_PARTITION_SUBTYPE_APP_OTA_15:
                return "OTA_15";
            case ESP_PARTITION_SUBTYPE_APP_OTA_MAX:
                return "OTA_MAX";
            default:
                return "UNKNOWN";
        }
    else
        switch(subtype) {
            case ESP_PARTITION_SUBTYPE_DATA_OTA:
                return "OTA";
            case ESP_PARTITION_SUBTYPE_DATA_PHY:
                return "PHY";
            case ESP_PARTITION_SUBTYPE_DATA_NVS:
                return "NVS";
            case ESP_PARTITION_SUBTYPE_DATA_COREDUMP:
                return "COREDUMP";
            case ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS:
                return "NVS_KEYS";
            case ESP_PARTITION_SUBTYPE_DATA_EFUSE_EM:
                return "EFUSE_EM";
            case ESP_PARTITION_SUBTYPE_DATA_ESPHTTPD:
                return "ESPHTTPD";
            case ESP_PARTITION_SUBTYPE_DATA_FAT:
                return "FAT";
            case ESP_PARTITION_SUBTYPE_DATA_SPIFFS:
                return "SPIFFS";
            default:
                return "UNKNOWN";
        }
}

cJSON* _get_partition_json(const esp_partition_t* partition) {
    cJSON* partition_json = cJSON_CreateObject();
    cJSON_AddStringToObject(partition_json, "label", partition->label);
    cJSON_AddStringToObject(partition_json, "type", _partition_type_to_str(partition->type));
    cJSON_AddStringToObject(partition_json, "subtype", _partition_subtype_to_str(partition->type, partition->subtype));
    cJSON_AddNumberToObject(partition_json, "address", partition->address);
    cJSON_AddNumberToObject(partition_json, "size", partition->size);
    cJSON_AddBoolToObject(partition_json, "encrypted", partition->encrypted);
    return partition_json;
}

esp_err_t _rmgmt_get_system_partitions(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");
    cJSON *partitions, *root = cJSON_CreateObject();
    partitions = cJSON_AddArrayToObject(root, "partitions");

    /// APP partitions
    esp_partition_iterator_t partition_it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);

    for (; partition_it != NULL; partition_it = esp_partition_next(partition_it)) {
        cJSON* partition_json = _get_partition_json(esp_partition_get(partition_it));
        cJSON_AddItemToArray(partitions, partition_json);
    }
    esp_partition_iterator_release(partition_it);

    /// DATA partitions
    partition_it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);

    for (; partition_it != NULL; partition_it = esp_partition_next(partition_it)) {
        cJSON* partition_json = _get_partition_json(esp_partition_get(partition_it));
        cJSON_AddItemToArray(partitions, partition_json);
    }
    esp_partition_iterator_release(partition_it);


    const char *partitions_json = cJSON_Print(root);
    ret = httpd_resp_sendstr(req, partitions_json);

    free((void *)partitions_json);
    cJSON_Delete(root);

    return ret;
}

esp_err_t _rmgmt_get_system_partitions_label(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");

    
    char label[PARTITION_LABEL_MAX_LENGTH];
    ret = _get_partition_label_from_uri(req->uri, label, PARTITION_LABEL_MAX_LENGTH);
    if(ret != ESP_OK) {
        //ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid partition label");
        return ESP_FAIL;
    }

    const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
    if(partition == NULL) {
        partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, label);
        if(partition == NULL){
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No partition found with given label");
            return ESP_FAIL;
        }
    }

    cJSON* root = _get_partition_json((const esp_partition_t*)partition);

    const char *partition_json = cJSON_Print(root);
    ret = httpd_resp_sendstr(req, partition_json);
    free((void *)partition_json);

    cJSON_Delete(root);

    return ret;
}

esp_err_t _rmgmt_put_system_partitions_label(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    


    const char *json = cJSON_Print(root);
    httpd_resp_sendstr(req, json);

    free((void *)json);
    cJSON_Delete(root);

    return ESP_OK;
}

esp_err_t _rmgmt_delete_system_partitions_label(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    


    const char *json = cJSON_Print(root);
    httpd_resp_sendstr(req, json);

    free((void *)json);
    cJSON_Delete(root);

    return ESP_OK;
}