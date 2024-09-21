#include "espnow_example.h"


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
    xTaskCreate(WS2812_Task,"WS2812_Task",2048,NULL,2,NULL);

    example_wifi_init();
    example_espnow_init();

}
