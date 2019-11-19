#include "rmgmt_server.h"


static const char* TAG = "RMGMT_SERVER";
static rmgmt_server_t* _rmgmt_server = NULL;

static void _connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    rmgmt_start(_rmgmt_server);
}

static void _disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    if(_rmgmt_server == NULL) return;
    rmgmt_stop(&_rmgmt_server->server_handle);
}


esp_err_t rmgmt_init(ssl_certs_t* ssl_certs) {
    rmgmt_release();

    esp_err_t err;

    // Initialize remote management server
    _rmgmt_server = (rmgmt_server_t*)malloc(sizeof(rmgmt_server_t));
    _rmgmt_server->server_handle = NULL;
    _rmgmt_server->endpoints = _rmgmt_endpoints;
    _rmgmt_server->endpoints_len = _rmgmt_endpoints_len;

    if(ssl_certs == NULL) {
        _rmgmt_server->ssl_certs = NULL;
    } else {
        _rmgmt_server->ssl_certs = (ssl_certs_t*)malloc(sizeof(ssl_certs_t));
        memcpy(_rmgmt_server->ssl_certs, ssl_certs, sizeof(ssl_certs_t));
    }

    // Setup event loop
    err = esp_event_loop_create_default();
    if(err == ESP_ERR_INVALID_STATE) err = ESP_OK;  // Already started
    if(err != ESP_OK) return err;

    // Register connection events
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_connect_handler, NULL);
    if(err != ESP_OK) return err;
    err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_disconnect_handler, NULL);
    return err;
    // TODO: Check if WIFI already connected
}

void rmgmt_release() {
    if(_rmgmt_server != NULL) {
        // Stop the server (just in case it was running)
        rmgmt_stop(&_rmgmt_server->server_handle);
        // Free SSL certs memory
        if(_rmgmt_server->ssl_certs != NULL)
            free(_rmgmt_server->ssl_certs);
        // Free RemoteManagementServer memory
        free(_rmgmt_server);
        _rmgmt_server = NULL;
    }
}


esp_err_t rmgmt_start(rmgmt_server_t* rmgmt_server) {
    if(rmgmt_server == NULL) return ESP_ERR_INVALID_ARG;
    if(rmgmt_server->server_handle != NULL) return ESP_ERR_INVALID_STATE;

    esp_err_t ret;
    ESP_LOGI(TAG, "Starting server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.httpd.uri_match_fn = httpd_uri_match_wildcard;
    conf.httpd.max_uri_handlers = _rmgmt_endpoints_len;

    if(rmgmt_server->ssl_certs == NULL) {
        conf.transport_mode = HTTPD_SSL_TRANSPORT_INSECURE;
    } else {
        conf.cacert_pem = rmgmt_server->ssl_certs->cacert;
        conf.cacert_len = rmgmt_server->ssl_certs->cacert_len;
        conf.prvtkey_pem = rmgmt_server->ssl_certs->prvtkey;
        conf.prvtkey_len = rmgmt_server->ssl_certs->prvtkey_len;
    }
    /*extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
    extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
    conf.cacert_pem = cacert_pem_start;
    conf.cacert_len = cacert_pem_end - cacert_pem_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;*/

    ret = httpd_ssl_start(&rmgmt_server->server_handle, &conf);
    if(ret != ESP_OK) return ret;
    
    for(int i = 0; i < rmgmt_server->endpoints_len; i++) {
        ESP_LOGI(TAG, "Registering URI: %s", rmgmt_server->endpoints[i].uri);
        ret = httpd_register_uri_handler(rmgmt_server->server_handle, &rmgmt_server->endpoints[i]);
        if(ret != ESP_OK) return ret;
    }
    return ret;
}

void rmgmt_stop(httpd_handle_t* server_handle) {
    if(*server_handle == NULL) return;

    httpd_ssl_stop(*server_handle);
    *server_handle = NULL;
}