#include <string.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "lwip/sockets.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define DEFAULT_SSID "ESP32-Setup"
#define WIFI_NVS_NAMESPACE "wifi-config"

static const char *TAG = "SSID_PORTAL";
static char custom_ssid[32] = DEFAULT_SSID;

static httpd_handle_t server = NULL;

// HTML form page
static const char *form_html =
    "<!DOCTYPE html><html><head><title>Set SSID</title></head><body>"
    "<h2>Change AP SSID</h2>"
    "<form action=\"/save\" method=\"POST\">"
    "SSID: <input type=\"text\" name=\"ssid\" required><br><br>"
    "<input type=\"submit\" value=\"Save and Restart\">"
    "</form><br>"
    "<form action=\"/reset\" method=\"POST\">"
    "<input type=\"submit\" value=\"Reset All\">"
    "</form></body></html>";

// Function prototype for url_decode
void url_decode(char *src);

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, form_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t save_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf) - 1));
    if (ret <= 0) return ESP_FAIL;

    buf[ret] = 0;
    char *ssid_start = strstr(buf, "ssid=");
    if (ssid_start) {
        ssid_start += 5;
        char *end = strchr(ssid_start, '&');
        if (end) *end = '\0';
        url_decode(ssid_start);

        strncpy(custom_ssid, ssid_start, sizeof(custom_ssid));

        nvs_handle_t nvs;
        nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
        nvs_set_str(nvs, "customSSID", custom_ssid);
        nvs_commit(nvs);
        nvs_close(nvs);

        httpd_resp_send(req, "SSID saved. Restarting...", HTTPD_RESP_USE_STRLEN);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        esp_restart();
    }
    return ESP_OK;
}

static esp_err_t reset_post_handler(httpd_req_t *req) {
    nvs_handle_t nvs;
    nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    nvs_erase_all(nvs);
    nvs_commit(nvs);
    nvs_close(nvs);

    httpd_resp_send(req, "Reset done. Restarting...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

static void start_ap() {
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    strncpy((char *)ap_config.ap.ssid, custom_ssid, sizeof(ap_config.ap.ssid));

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Access Point started with SSID: %s", custom_ssid);
}

static void start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_start(&server, &config);

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler
    };
    httpd_register_uri_handler(server, &root);

    httpd_uri_t save = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = save_post_handler
    };
    httpd_register_uri_handler(server, &save);

    httpd_uri_t reset = {
        .uri = "/reset",
        .method = HTTP_POST,
        .handler = reset_post_handler
    };
    httpd_register_uri_handler(server, &reset);
}

void url_decode(char *src) {
    char *dst = src;
    while (*src) {
        if ((*src == '%') && ((src[1]) && (src[2]))) {
            char hex[3] = { src[1], src[2], '\0' };
            *dst++ = (char) strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void app_main() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();

    nvs_handle_t nvs;
    size_t len = sizeof(custom_ssid);
    if (nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_str(nvs, "customSSID", custom_ssid, &len);
        nvs_close(nvs);
    }

    start_ap();
    start_webserver();
}
