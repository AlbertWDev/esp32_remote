#include "status.h"


esp_err_t fill_mac_address(wifi_interface_t ifx, cJSON* node) {
    uint8_t mac[6];
    esp_wifi_get_mac(ifx, mac);
    char* mac_str = (char*) malloc(18);
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(node, "mac", mac_str);
    
    return ESP_OK;
}

esp_err_t fill_ip_address(tcpip_adapter_if_t tcpip_if, cJSON* node) {
    tcpip_adapter_ip_info_t ipInfo;
    tcpip_adapter_get_ip_info(tcpip_if, &ipInfo);
    cJSON_AddStringToObject(node, "ip", inet_ntoa(ipInfo.ip));
    cJSON_AddStringToObject(node, "gw", inet_ntoa(ipInfo.gw));
    cJSON_AddStringToObject(node, "netmask", inet_ntoa(ipInfo.netmask));

    return ESP_OK;
}

esp_err_t _rmgmt_get_network_wifi_status(httpd_req_t *req) {
    ALLOW_CORS(req);
    esp_err_t ret;
    
    cJSON *sta, *ap, *root = cJSON_CreateObject();
    sta = cJSON_AddObjectToObject(root, "sta");
    ap = cJSON_AddObjectToObject(root, "ap");

    bool init = false;

    wifi_mode_t mode;
	ret = esp_wifi_get_mode(&mode);
    cJSON_AddBoolToObject(root, "init", (init = (ret != ESP_ERR_WIFI_NOT_INIT)));

    if(!init)
        cJSON_AddStringToObject(root, "mode", "NULL");
    else {
        switch (mode) {
            case WIFI_MODE_NULL:
                cJSON_AddStringToObject(root, "mode", "NULL"); break;
            case WIFI_MODE_STA:
                cJSON_AddStringToObject(root, "mode", "STA"); break;
            case WIFI_MODE_AP:
                cJSON_AddStringToObject(root, "mode", "AP"); break;
            case WIFI_MODE_APSTA:
                cJSON_AddStringToObject(root, "mode", "APSTA"); break;
            default:
                cJSON_AddStringToObject(root, "mode", "unknown"); break;
        }


        wifi_config_t conf;

        fill_mac_address(WIFI_IF_STA, sta);
        fill_ip_address(TCPIP_ADAPTER_IF_STA, sta);
	    esp_wifi_get_config(WIFI_IF_STA, &conf);
        cJSON_AddStringToObject(sta, "ssid", (char*)conf.sta.ssid);

        fill_mac_address(WIFI_IF_AP, ap);
        fill_ip_address(TCPIP_ADAPTER_IF_AP, ap);
	    esp_wifi_get_config(WIFI_IF_AP, &conf);
        cJSON_AddStringToObject(ap, "ssid", (char*)conf.ap.ssid);
    }

    const char *wifi_json = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, wifi_json);

    free((void *)wifi_json);
    cJSON_Delete(root);

    return ESP_OK;
}

esp_err_t _rmgmt_post_network_wifi_status(httpd_req_t *req) {
    ALLOW_CORS(req);

    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not implemented");
    return ESP_OK;
}