#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_station.h"
#include <stdio.h>
#include <string.h>

const char* firmware_version = "v1.1.0";

extern const uint8_t server_cert_pem_start[] asm("_binary_google_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_google_pem_end");

static void nvs_init(void);
static void run_ota(void);

static esp_err_t client_event_handle(esp_http_client_event_t* evt);

void app_main(void)
{
    nvs_init();
    wifi_init_sta();
    run_ota();
}

static void run_ota(void)
{

    esp_http_client_config_t client_config = {
        .url               = "https://mq-esp32-ota.s3.amazonaws.com/esp32-ota.bin", // ota locaiton;
        .event_handler     = client_event_handle,
        .cert_pem          = (char*)server_cert_pem_start,
        .timeout_ms        = 20000, // default is 5000
        .keep_alive_enable = true,

    };

    esp_https_ota_config_t ota_config = {
        .http_config = &client_config,
    };

    esp_https_ota_handle_t ota_handle = NULL;

    if (esp_https_ota_begin(&ota_config, &ota_handle) != ESP_OK)
    {
        ESP_LOGE(__FUNCTION__, "fail to find ota file");
        return;
    }

    // get this app description
    const esp_partition_t* esp_partition = esp_ota_get_running_partition();
    esp_app_desc_t         esp_app_description;
    esp_ota_get_partition_description(esp_partition, &esp_app_description);

    // get ota image app description
    esp_app_desc_t incoming_esp_app_description;
    esp_https_ota_get_img_desc(ota_handle, &incoming_esp_app_description);

    // compare both apps description.  download if different;
    if (strcmp((const char*)&incoming_esp_app_description.version, (const char*)&esp_app_description.version) == 0)
    {
        ESP_LOGI(__FUNCTION__, "app version = %s", esp_app_description.version);
        esp_https_ota_abort(ota_handle);
        return;
    }

    ESP_LOGI(__FUNCTION__, "app version = %s, update to new app version = %s", esp_app_description.version,
             incoming_esp_app_description.version);

    esp_err_t ota_result;
    do
    {
        ota_result = esp_https_ota_perform(ota_handle);
    } while (ota_result == ESP_ERR_HTTPS_OTA_IN_PROGRESS);

    ESP_ERROR_CHECK(ota_result);
    ESP_ERROR_CHECK(esp_https_ota_finish(ota_handle));
    esp_restart();
}

static esp_err_t client_event_handle(esp_http_client_event_t* evt)
{
    return ESP_OK;
}

static void nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
}