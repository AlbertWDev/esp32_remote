#include "partitions.h"

#define SHA256_HASH_LEN 32
#define PARTITION_LABEL_MAX_LENGTH 16
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

    cJSON* root = _get_partition_json((const esp_partition_t*)partition);

    const char *partition_json = cJSON_Print(root);
    ret = httpd_resp_sendstr(req, partition_json);
    free((void *)partition_json);

    cJSON_Delete(root);

    return ret;
}

esp_err_t _rmgmt_get_system_partitions_label_sha256(httpd_req_t *req) {
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
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not implemented");
    return ESP_FAIL;
}

esp_err_t _rmgmt_put_system_partitions_label(httpd_req_t *req) {
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

    /*#define PARTITION_OTA_BUFFER_SIZE 1024
    int bytes_read;
    char data[PARTITION_OTA_BUFFER_SIZE] = {0};
    do {
        bytes_read = httpd_req_recv(req, data, PARTITION_OTA_BUFFER_SIZE);

    */

    esp_ota_handle_t update_handle = 0;

    #define BUFFSIZE 1024
    char ota_write_data[BUFFSIZE + 1] = { 0 };

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;

    
    while (1) {
        int data_read = httpd_req_recv(req, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            // ERROR
            // TODO: Cleanup
            return ESP_OK;
        } else if (data_read > 0) {
            /*if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG, "New version is the same as invalid version.");
                            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                            http_cleanup(client);
                            infinite_loop();
                        }
                    }

                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                        http_cleanup(client);
                        infinite_loop();
                    }

                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        task_fatal_error();
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG, "received package is not fit len");
                    http_cleanup(client);
                    task_fatal_error();
                }
            }*/
            if(!image_header_was_checked) {
                ret = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &update_handle);
                if (ret != ESP_OK) {
                    ESP_LOGE("PARTITIONS", "esp_ota_begin failed (%s)", esp_err_to_name(ret));
                    return ESP_OK;
                }
                ESP_LOGI("PARTITIONS", "esp_ota_begin succeeded");
            }
            ret = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
            if (ret != ESP_OK) {
                ESP_LOGE("PARTITIONS", "esp_ota_write failed");
                return ESP_OK;
            }
            binary_file_length += data_read;
            ESP_LOGD("PARTITIONS", "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
            ESP_LOGI("PARTITIONS", "Connection closed");
            break;
        }
    }
    ESP_LOGI("PARTITIONS", "Total Write binary data length: %d", binary_file_length);
    /*if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG, "Error in receiving complete file");
        http_cleanup(client);
        task_fatal_error();
    }*/

    ret = esp_ota_end(update_handle);
    if (ret != ESP_OK) {
        ESP_LOGE("PARTITIONS", "esp_ota_end failed (%s)!", esp_err_to_name(ret));
        return ESP_OK;
    }

    /*err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();*/



    /*httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    const char *json = cJSON_Print(root);
    httpd_resp_sendstr(req, json);

    free((void *)json);
    cJSON_Delete(root);*/

    return ESP_OK;
}

esp_err_t _rmgmt_put_system_partitions_label_boot(httpd_req_t *req) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not implemented");
    return ESP_FAIL;
}

esp_err_t _rmgmt_post_system_partitions_label_validate(httpd_req_t *req) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not implemented");
    return ESP_FAIL;
}

esp_err_t _rmgmt_delete_system_partitions_label(httpd_req_t *req) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not implemented");
    return ESP_FAIL;
}