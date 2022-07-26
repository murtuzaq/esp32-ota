#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_station.h"
#include <stdio.h>
#define TASK_OTA_TRIGGER_STACK_SIZE 1024 * 8

bool        ota_trigger_flag = false;
const char* firmware_version = "v1.1.0";

extern const uint8_t server_cert_pem_start[] asm("_binary_google_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_google_pem_end");

static void      task_ota_trigger(void* pvParameters);
static esp_err_t client_event_handle(esp_http_client_event_t* evt);

void app_main(void)
{
    const esp_partition_t* esp_partition = esp_ota_get_running_partition();

    esp_app_desc_t esp_app_description;
    esp_ota_get_partition_description(esp_partition, &esp_app_description);

    ESP_LOGI(__FUNCTION__, "app version = %s", esp_app_description.version);

    xTaskCreate(task_ota_trigger, "task_ota_trigger", TASK_OTA_TRIGGER_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
}

static void task_ota_trigger(void* pvParameters)
{
    while (1)
    {
        vTaskDelay(1000);

        ota_trigger_flag = true;
        ESP_LOGW(__FUNCTION__, "ota triggered");

        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }

        wifi_init_sta();

        esp_http_client_config_t client_config = {
            .url               = "https://mq-esp32-ota.s3.amazonaws.com/esp32-ota.bin", // ota locaiton;
            .event_handler     = client_event_handle,
            .cert_pem          = (char*)server_cert_pem_start,
            .timeout_ms        = 20000, // default is 5000
            .keep_alive_enable = true,

        };

        esp_err_t ota_err = esp_https_ota(&client_config);

        if (ota_err == ESP_OK)
        {
            ESP_LOGI(__FUNCTION__, "OTA flash successful for version %s", firmware_version);
            ESP_LOGI(__FUNCTION__, "reboot in 5 seconds");
            vTaskDelay(500);
            esp_restart();
        }

        ESP_LOGE(__FUNCTION__, "OTA flash not successful for version %s", firmware_version);
        ESP_ERROR_CHECK(ota_err);

        vTaskDelete(NULL);
    }
}

static esp_err_t client_event_handle(esp_http_client_event_t* evt)
{
    return ESP_OK;
}