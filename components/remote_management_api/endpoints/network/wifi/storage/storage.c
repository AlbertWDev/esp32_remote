#include "storage.h"

#define NETWORK_SSID_MAX_LENGTH 32
#define NETWORK_WIFI_MAX_LENGTH 256

esp_err_t _get_ssid_from_uri(const char* uri, char* ssid, size_t max_len) {
    size_t uri_len = strlen(uri);
    size_t offset = strlen("/v1/network/wifi/storage/");

    if(offset >= uri_len)
        return RMGMT_ERR_NETWORK_WIFI_SSID_INVALID;
    
    strncpy(ssid, uri + offset, max_len - 1);
    ssid[max_len - 1] = '\0';
    return ESP_OK;
}


esp_err_t _get_network_info_from_req(httpd_req_t *req, wm_network_info_t* network) {
    if(req == NULL || network == NULL)
        return ESP_ERR_INVALID_ARG;
    
    // Receive request content
    if(req->content_len >= NETWORK_WIFI_MAX_LENGTH)
        return RMGMT_ERR_NETWORK_WIFI_REQ_CONTENT_TOO_LONG;
    
    char* req_content = malloc(NETWORK_WIFI_MAX_LENGTH);
    
    int received, total_received = 0;
    while(total_received < req->content_len) {
        total_received += (received = httpd_req_recv(req, req_content, req->content_len));
        if(received <= 0) { // Connection closed, timeout or other error
            free(req_content);
            return RMGMT_ERR_NETWORK_WIFI_REQ_CONTENT_NOT_RECEIVED;
        }
    }
    req_content[req->content_len] = '\0';


    // Parse request content as JSON
    cJSON* network_json = cJSON_Parse(req_content);
    free(req_content);
    if(network_json == NULL)
    {
        cJSON_Delete(network_json);
        return RMGMT_ERR_NETWORK_WIFI_REQ_CONTENT_INVALID;
    }

    // Get network attributes from JSON
    cJSON* ssid = cJSON_GetObjectItemCaseSensitive(network_json, "ssid");
    if(!cJSON_IsString(ssid) || (ssid->valuestring == NULL)) {
        cJSON_Delete(network_json);
        return RMGMT_ERR_NETWORK_WIFI_SSID_REQUIRED;
    }
    strcpy(network->ssid, ssid->valuestring);

    cJSON* password = cJSON_GetObjectItemCaseSensitive(network_json, "password");
    if(!cJSON_IsString(password) || (password->valuestring == NULL))
        network->password[0] = '\0';
    else
        strcpy(network->password, password->valuestring);
    
    network->times_used = 0;
    
    cJSON_Delete(network_json);
    return ESP_OK;
}

esp_err_t _network_info_error_handler(httpd_req_t* req, esp_err_t err) {
    switch(err) {
        case RMGMT_ERR_NETWORK_WIFI_REQ_CONTENT_TOO_LONG:
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long");
            return ESP_OK;
        case RMGMT_ERR_NETWORK_WIFI_REQ_CONTENT_NOT_RECEIVED:
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Couldn't receive request content");
            return ESP_OK;
        case RMGMT_ERR_NETWORK_WIFI_REQ_CONTENT_INVALID:
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request content not recognized as JSON");
            return ESP_OK;
        case RMGMT_ERR_NETWORK_WIFI_SSID_REQUIRED:
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
            return ESP_OK;
        default:
            return ESP_FAIL;
    }
}

cJSON* _get_network_json(wm_network_info_t* network) {
    cJSON* network_json = cJSON_CreateObject();

    cJSON_AddStringToObject(network_json, "ssid", network->ssid);
    cJSON_AddStringToObject(network_json, "password", "********");//network->password);
    cJSON_AddNumberToObject(network_json, "score", network->times_used);

    return network_json;
}

esp_err_t _rmgmt_get_network_wifi_storage(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    cJSON *network_list = cJSON_CreateArray();
    
    wm_network_info_t* networks = (wm_network_info_t*)malloc(WM_STORAGE_MAX_NETWORKS*sizeof(wm_network_info_t));
    size_t networks_count = WM_STORAGE_MAX_NETWORKS;
    ret = wm_storage_read(networks, &networks_count);
    if(ret != ESP_OK) { free(networks); cJSON_Delete(network_list); return ret; }

    for(int i = 0; i < networks_count; i++) {
        cJSON* network_json = _get_network_json(&networks[i]);
        cJSON_AddItemToArray(network_list, network_json);
    }

    free(networks);

    const char *json = cJSON_Print(network_list);
    httpd_resp_sendstr(req, json);

    free((void *)json);
    cJSON_Delete(network_list);

    return ESP_OK;
}

esp_err_t _rmgmt_get_network_wifi_storage_ssid(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char ssid[NETWORK_SSID_MAX_LENGTH];
    ret = _get_ssid_from_uri(req->uri, ssid, NETWORK_SSID_MAX_LENGTH);
    if(ret != ESP_OK) {
        //ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid network ssid");
        return ESP_FAIL;
    }

    wm_network_info_t* networks = (wm_network_info_t*)malloc(WM_STORAGE_MAX_NETWORKS*sizeof(wm_network_info_t));
    size_t networks_count = WM_STORAGE_MAX_NETWORKS;
    ret = wm_storage_read(networks, &networks_count);
    if(ret != ESP_OK) { free(networks); return ret; }

    bool found = false;
    for(int i = 0; i < networks_count; i++) {
        if(strcmp(ssid, networks[i].ssid) == 0) {
            cJSON* network = _get_network_json(&networks[i]);
            const char* network_json = cJSON_Print(network);

            httpd_resp_sendstr(req, network_json);

            free((void*)network_json);
            cJSON_Delete(network);
            found = true;
            break;
        }
    }
    free(networks);

    if(!found) httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "No network found with given SSID");

    return ESP_OK;
}

esp_err_t _rmgmt_post_network_wifi_storage(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    wm_network_info_t network;
    ret = _get_network_info_from_req(req, &network);
    if(ret != ESP_OK) return _network_info_error_handler(req, ret);


    // TODO: Check if network already exists

    wm_storage_save(&network);
    

    cJSON* network_json = _get_network_json(&network);
    const char *json = cJSON_Print(network_json);
    
    httpd_resp_sendstr(req, json);
    
    free((void *)json);
    cJSON_Delete(network_json);

    return ESP_OK;
}

esp_err_t _rmgmt_put_network_wifi_storage_ssid(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    cJSON *root = cJSON_CreateObject();
    


    const char *json = cJSON_Print(root);
    httpd_resp_sendstr(req, json);

    free((void *)json);
    cJSON_Delete(root);

    return ESP_OK;
}

esp_err_t _rmgmt_delete_network_wifi_storage_ssid(httpd_req_t *req) {
    esp_err_t ret;
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    //wm_network_info_t network;
    //ret = _get_network_info_from_req(req, &network);
    //if(ret != ESP_OK) return _network_info_error_handler(req, ret);
    char ssid[NETWORK_SSID_MAX_LENGTH];
    ret = _get_ssid_from_uri(req->uri, ssid, NETWORK_SSID_MAX_LENGTH);
    if(ret != ESP_OK) {
        //ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid network ssid");
        return ESP_FAIL;
    }

    ret = wm_storage_delete(ssid);
    if(ret != ESP_OK) {
        if(ret == ESP_ERR_NVS_NOT_FOUND) {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Network not found with given SSID");
            return ESP_OK;
        } else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Couldn't delete stored network");
            return ESP_FAIL;
        }
    }
    
    httpd_resp_sendstr(req, "Network deleted");
    return ESP_OK;
}
