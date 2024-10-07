#include "espnow_example.h"
QueueHandle_t ADC_queue;
static const char *TAG = "MAIN INIT";

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    board_init();

    ADC_queue = xQueueCreate(100,8);
    if(ADC_queue == 0)
    {
        ESP_LOGE(TAG,"queue create failed!");
    }

    xTaskCreate(WS2812_Task,"WS2812_Task",3072,NULL,2,NULL);
    xTaskCreate(ADC_Task,"ADC_Task",4096,NULL,2,NULL);


    ret = wifi_sta_init();
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG,"ESP WIFI init failed!");
    }
    
    example_espnow_init();

    
}
