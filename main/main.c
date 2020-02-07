#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "tcpip_adapter.h"
#include "esp_eth.h"

#include <esp_vfs.h>
#include <esp_spiffs.h>

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"

#include "remote_management.h"
#include "headers.h"
#include "wifi_manager.h"

#include <esp_log.h>

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   4


static const char *TAG = "MAIN";

esp_err_t _get_battery(httpd_req_t* req) {
    APPLY_HEADERS(req);
    httpd_resp_sendstr(req, "{\"level\":50.0}");
    return ESP_OK;
}

void app_main() {
    // Initialize SPIFFS
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "storage",
      .max_files = 5,
      .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }
    
    // Initialize SD card
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = VSPI_HOST;
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = PIN_NUM_MISO;
    slot_config.gpio_mosi = PIN_NUM_MOSI;
    slot_config.gpio_sck  = PIN_NUM_CLK;
    slot_config.gpio_cs   = PIN_NUM_CS;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
    }


    extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
    extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    ssl_certs_t ssl_certs = {
        .cacert = cacert_pem_start,
        .cacert_len = cacert_pem_end - cacert_pem_start,
        .prvtkey = prvtkey_pem_start,
        .prvtkey_len = prvtkey_pem_end - prvtkey_pem_start
    };
    const size_t user_endpoints_len = 1;
    const httpd_uri_t user_endpoints[] = {
        /*** System.Battery ***/
        {
            .uri        = "/v1/system/battery/?",
            .method     = HTTP_GET,
            .handler    = _get_battery
        }
    };
    rmgmt_init(&ssl_certs, user_endpoints, user_endpoints_len);

    wm_config_t wm_config = {
        .ap_ssid = "esp32_testing",
        .ap_password = "4321gnitset",
        .version = 0x01
    };
    wm_init(&wm_config);
}
