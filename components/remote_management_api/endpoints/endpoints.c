#include "../remote_management.h"

#include "system/info/info.h"
#include "system/partitions/partitions.h"
#include "network/wifi/storage/storage.h"
#include "network/wifi/status/status.h"
#include "storage/storage.h"
//#include "gpio/gpio.h"

const size_t _rmgmt_endpoints_len = 20;
const httpd_uri_t _rmgmt_endpoints[] = {
    /*** System.Info ***/
    {
        .uri        = "/v1/system/info/?",
        .method     = HTTP_GET,
        .handler    = _rmgmt_get_system_info
    },
    /*** System.Partitions ***/
    {
        .uri        = "/v1/system/partitions/?",
        .method     = HTTP_GET,
        .handler    = _rmgmt_get_system_partitions
    },
    {
        .uri        = "/v1/system/partitions/?",
        .method     = HTTP_PUT,
        .handler    = _rmgmt_put_system_partitions
    },
    {
        .uri        = "/v1/system/partitions/*/sha256/?",
        .method     = HTTP_GET,
        .handler    = _rmgmt_get_system_partitions_label_sha256
    },
    {
        .uri        = "/v1/system/partitions/*/boot/?",
        .method     = HTTP_PUT,
        .handler    = _rmgmt_put_system_partitions_label_boot
    },
    {
        .uri        = "/v1/system/partitions/*/validate/?",
        .method     = HTTP_POST,
        .handler    = _rmgmt_post_system_partitions_label_validate
    },
    {
        .uri        = "/v1/system/partitions/*",
        .method     = HTTP_GET,
        .handler    = _rmgmt_get_system_partitions_label
    },
    {
        .uri        = "/v1/system/partitions/*",
        .method     = HTTP_PUT,
        .handler    = _rmgmt_put_system_partitions_label
    },
    {
        .uri        = "/v1/system/partitions/*",
        .method     = HTTP_DELETE,
        .handler    = _rmgmt_delete_system_partitions_label
    },
    /*** Network.WiFi.Storage ***/
    {
        .uri        = "/v1/network/wifi/storage/?",
        .method     = HTTP_GET,
        .handler    = _rmgmt_get_network_wifi_storage
    },
    {
        .uri        = "/v1/network/wifi/storage/*",
        .method     = HTTP_GET,
        .handler    = _rmgmt_get_network_wifi_storage_ssid
    },
    {
        .uri        = "/v1/network/wifi/storage/?",
        .method     = HTTP_POST,
        .handler    = _rmgmt_post_network_wifi_storage
    },
    {
        .uri        = "/v1/network/wifi/storage/*",
        .method     = HTTP_PUT,
        .handler    = _rmgmt_put_network_wifi_storage_ssid
    },
    {
        .uri        = "/v1/network/wifi/storage/*",
        .method     = HTTP_DELETE,
        .handler    = _rmgmt_delete_network_wifi_storage_ssid
    },
    /*** Network.WiFi.Status ***/
    {
        .uri        = "/v1/network/wifi/status/?",
        .method     = HTTP_GET,
        .handler    = _rmgmt_get_network_wifi_status
    },
    {
        .uri        = "/v1/network/wifi/status/?",
        .method     = HTTP_PUT,
        .handler    = _rmgmt_put_network_wifi_status
    },
    /*** Storage ***/
    {
        .uri        = "/v1/storage/?",
        .method     = HTTP_GET,
        .handler    = _rmgmt_get_storage
    },
    {
        .uri        = "/v1/storage/*",
        .method     = HTTP_GET,
        .handler    = _rmgmt_get_storage_node
    },
    {
        .uri        = "/v1/storage/*",
        .method     = HTTP_POST,
        .handler    = _rmgmt_post_storage_node
    },
    {
        .uri        = "/v1/storage/*",
        .method     = HTTP_PUT,
        .handler    = _rmgmt_put_storage_node
    },
    {
        .uri        = "/v1/storage/*",
        .method     = HTTP_DELETE,
        .handler    = _rmgmt_delete_storage_node
    }/*,
    {
        .uri        = "/gpio/?",
        .method     = HTTP_GET,
        .handler    = rest_handler_get_gpio
    }*/
};