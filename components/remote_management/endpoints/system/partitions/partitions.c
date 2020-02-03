#include "partitions.h"

#define SHA256_HASH_LEN 32
#define PARTITION_LABEL_MAX_LENGTH 16
#define PARTITION_OTA_UPDATE_BUFFER_SIZE 1024
#define REST_ERR_STORAGE_PARTITION_INVALID_LABEL 0x70

esp_err_t _get_partition_label_from_uri(const char* uri, const char* tail, char* label, size_t max_len) {
    size_t tail_len = 0;
    if(tail != NULL) tail_len = strlen(tail);
    
    size_t uri_len = strlen(uri) - tail_len;
    size_t offset = strlen("/v1/system/partitions/");

    // Check if endpoint base overlaps URI
    if(offset >= uri_len)
        return REST_ERR_STORAGE_PARTITION_INVALID_LABEL;

    // Calculate partition label length without exceeding maximum
    size_t label_len = uri_len - offset;
    if(label_len > max_len - 1)
        label_len = max_len - 1;

    strncpy(label, uri + offset, label_len);
    label[label_len] = '\0';
    return ESP_OK;
}

static const char* _partition_type_to_str(esp_partition_type_t type) {
    switch(type) {
        case ESP_PARTITION_TYPE_APP:
            return "APP";
        case ESP_PARTITION_TYPE_DATA:
            return "DATA";
        default:
            return "UNKNOWN";
    }
}

static const char* _partition_subtype_to_str(esp_partition_type_t type, esp_partition_subtype_t subtype) {
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

esp_err_t _ota_update_app_partition(const esp_partition_t* partition, httpd_req_t* req) {
    esp_err_t ret = ESP_OK;

    int bytes_read;
    char data[PARTITION_OTA_UPDATE_BUFFER_SIZE] = {0};
    size_t bytes_writen = 0;
    esp_ota_handle_t update_handle;

    while(1) {
        // Receive image data
        bytes_read = httpd_req_recv(req, data, PARTITION_OTA_UPDATE_BUFFER_SIZE);
        if(bytes_read <= 0) break;

        if(bytes_writen == 0) { // First packet
            // TODO: Check version
            /*if (bytes_read < sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                ret = ESP_FAIL;
                break;
            }
            esp_app_desc_t new_app_info;
            
            // Get runnning and downloaded app versions
            memcpy(&new_app_info, &data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));

            const esp_partition_t* running = esp_ota_get_running_partition();
            esp_app_desc_t running_app_info;
            ret = esp_ota_get_partition_description(running, &running_app_info);
            if(ret != ESP_OK) break;

            if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                // Same version
                break;
            }*/

            ret = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &update_handle);
            if (ret != ESP_OK) break;
        }

        // Write image data
        ret = esp_ota_write(update_handle, (const void *)data, bytes_read);
        if (ret != ESP_OK) break;

        bytes_writen += bytes_read;
    }

    // Check if something went wrong
    if(bytes_read < 0 || ret != ESP_OK || bytes_writen < req->content_len) {
        esp_ota_end(update_handle);
        return ESP_FAIL;
    }

    return esp_ota_end(update_handle);
}

esp_err_t _ota_update_data_partition(const esp_partition_t* partition, httpd_req_t* req) {
    esp_err_t ret = ESP_OK;

    int bytes_read;
    char data[PARTITION_OTA_UPDATE_BUFFER_SIZE] = {0};
    size_t bytes_writen = 0;

    while(1) {
        // Receive image data
        bytes_read = httpd_req_recv(req, data, PARTITION_OTA_UPDATE_BUFFER_SIZE);
        if(bytes_read <= 0) break;

        if(bytes_writen == 0) { // First packet
            ret = esp_partition_erase_range(partition, 0, partition->size);
            if (ret != ESP_OK) break;
        }

        // Write image data
        ret = esp_partition_write(partition, bytes_writen, (const void *)data, bytes_read);
        if (ret != ESP_OK) break;

        bytes_writen += bytes_read;
    }
    
    // Check if something went wrong
    if(bytes_read < 0 || ret != ESP_OK || bytes_writen < req->content_len)
        return ESP_FAIL;
    
    return ret;
}

esp_err_t _rmgmt_get_system_partitions(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;

    httpd_resp_set_type(req, "application/json");
    cJSON *partitions = cJSON_CreateArray();

    /// APP partitions
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* boot = esp_ota_get_boot_partition();

    esp_partition_iterator_t partition_it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);

    for (; partition_it != NULL; partition_it = esp_partition_next(partition_it)) {
        const esp_partition_t* partition = esp_partition_get(partition_it);
        cJSON* partition_json = _get_partition_json(partition);

        if(running != NULL && running == partition)
            cJSON_AddBoolToObject(partition_json, "running", true);
        if(boot != NULL && boot == partition)
            cJSON_AddBoolToObject(partition_json, "boot", true);

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


    const char *partitions_json = cJSON_Print(partitions);
    ret = httpd_resp_sendstr(req, partitions_json);

    free((void *)partitions_json);
    cJSON_Delete(partitions);
    return ret;
}

esp_err_t _rmgmt_get_system_partitions_label(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;
    
    char label[PARTITION_LABEL_MAX_LENGTH];
    ret = _get_partition_label_from_uri(req->uri, NULL, label, PARTITION_LABEL_MAX_LENGTH);
    if(ret != ESP_OK) {
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

    cJSON* partition_info = _get_partition_json((const esp_partition_t*)partition);

    const char *partition_json = cJSON_Print(partition_info);
    httpd_resp_set_type(req, "application/json");
    ret = httpd_resp_sendstr(req, partition_json);

    free((void *)partition_json);
    cJSON_Delete(partition_info);
    return ret;
}

esp_err_t _rmgmt_get_system_partitions_label_sha256(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;

    // Get label from URI
    char label[PARTITION_LABEL_MAX_LENGTH];
    ret = _get_partition_label_from_uri(req->uri, "/sha256", label, PARTITION_LABEL_MAX_LENGTH);
    if(ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid partition label");
        return ESP_FAIL;
    }

    // Get partition with given label
    const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
    if(partition == NULL) {
        partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, label);
        if(partition == NULL){
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No partition found with given label");
            return ESP_FAIL;
        }
    }
    
    // Calculate SHA-256
    uint8_t sha256[SHA256_HASH_LEN] = { 0 };
    ret = esp_partition_get_sha256(partition, sha256);
    if(ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Can't calculate SHA-256 digest of this partition");
        return ESP_FAIL;
    }

    // Stringify SHA-256
    char sha256_str[SHA256_HASH_LEN * 2 + 1];
    for(int i = 0; i < SHA256_HASH_LEN; i++)
        sprintf(&sha256_str[i * 2], "%02x", sha256[i]);
    sha256_str[SHA256_HASH_LEN * 2] = '\0';

    // Add to JSON object
    cJSON* sha256_json = cJSON_CreateObject();
    cJSON_AddStringToObject(sha256_json, "sha256", sha256_str);

    // Stringify JSON object
    const char *json = cJSON_Print(sha256_json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);

    free((void *)json);
    cJSON_Delete(sha256_json);
    return ESP_OK;
}

esp_err_t _rmgmt_put_system_partitions(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;

    const esp_partition_t* partition = esp_ota_get_next_update_partition(NULL);
    if(partition == NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No OTA app slot found");
        return ESP_FAIL;
    }

    ret = _ota_update_app_partition(partition, req);
    if(ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA update failed");
        return ret;
    }

    cJSON* partition_info = _get_partition_json((const esp_partition_t*)partition);

    const char *partition_json = cJSON_Print(partition_info);
    httpd_resp_set_type(req, "application/json");
    ret = httpd_resp_sendstr(req, partition_json);

    free((void *)partition_json);
    cJSON_Delete(partition_info);
    return ret;
}

esp_err_t _rmgmt_put_system_partitions_label(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;

    char label[PARTITION_LABEL_MAX_LENGTH];
    ret = _get_partition_label_from_uri(req->uri, NULL, label, PARTITION_LABEL_MAX_LENGTH);
    if(ret != ESP_OK) {
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

    if(partition->type == ESP_PARTITION_TYPE_APP)
        // App partition upload
        ret = _ota_update_app_partition(partition, req);
    else
        // Other partition upload
        ret = _ota_update_data_partition(partition, req);

    if(ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA update failed");
        return ret;
    }

    httpd_resp_sendstr(req, "Upload success");
    return ESP_OK;
}

esp_err_t _rmgmt_put_system_partitions_label_boot(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;

    // Get label from URI
    char label[PARTITION_LABEL_MAX_LENGTH];
    ret = _get_partition_label_from_uri(req->uri, "/boot", label, PARTITION_LABEL_MAX_LENGTH);
    if(ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid partition label");
        return ESP_FAIL;
    }

    // Get partition with given label
    const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
    if(partition == NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No APP partition found with given label");
        return ESP_FAIL;
    }

    // Set partition as boot
    ret = esp_ota_set_boot_partition(partition);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Could not set this partition as boot");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Boot set");
    return ESP_OK;
}

esp_err_t _rmgmt_post_system_partitions_label_validate(httpd_req_t *req) {
    APPLY_HEADERS(req);

    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not implemented");
    return ESP_FAIL;
}

esp_err_t _rmgmt_delete_system_partitions_label(httpd_req_t *req) {
    APPLY_HEADERS(req);
    esp_err_t ret;

    // Get label from URI
    char label[PARTITION_LABEL_MAX_LENGTH];
    ret = _get_partition_label_from_uri(req->uri, NULL, label, PARTITION_LABEL_MAX_LENGTH);
    if(ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid partition label");
        return ESP_FAIL;
    }

    // Get partition with given label
    const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
    if(partition == NULL) {
        partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, label);
        if(partition == NULL){
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No partition found with given label");
            return ESP_FAIL;
        }
    }

    // Erase partition
    ret = esp_partition_erase_range(partition, 0, partition->size);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not erase partition");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Deleted successfully");
    return ESP_OK;
}