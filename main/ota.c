#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include <string.h>

extern const uint8_t server_cert_pem_start[] asm("_binary_google_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_google_pem_end");

static esp_https_ota_handle_t begin_ota(void);
static bool                   is_ota_version_new(esp_https_ota_handle_t ota_handle);
static void                   ota_new_firmware(esp_https_ota_handle_t ota_handle);
static esp_err_t              client_event_handle(esp_http_client_event_t* evt);

void ota_run(void)
{
    esp_https_ota_handle_t ota_handle = begin_ota();
    if (ota_handle == NULL)
    {
        return;
    }

    if (is_ota_version_new(ota_handle) == false)
    {
        return;
    }

    ota_new_firmware(ota_handle);
}

static esp_https_ota_handle_t begin_ota(void)
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
    }

    return ota_handle;
}

static bool is_ota_version_new(esp_https_ota_handle_t ota_handle)
{
    const esp_partition_t* esp_partition = esp_ota_get_running_partition();
    esp_app_desc_t         esp_app_description;
    esp_ota_get_partition_description(esp_partition, &esp_app_description);

    esp_app_desc_t incoming_esp_app_description;
    esp_https_ota_get_img_desc(ota_handle, &incoming_esp_app_description);

    if (strcmp((const char*)&incoming_esp_app_description.version, (const char*)&esp_app_description.version) == 0)
    {
        ESP_LOGI(__FUNCTION__, "app version = %s", esp_app_description.version);
        esp_https_ota_abort(ota_handle);
        return false;
    }

    ESP_LOGI(__FUNCTION__, "app version = %s, update to new app version = %s", esp_app_description.version,
             incoming_esp_app_description.version);

    return true;
}

static void ota_new_firmware(esp_https_ota_handle_t ota_handle)
{
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