#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "board.h"
#include "ws2812/ws2812.h"
#include "esp_now_app.h"
#include "adc_detect/adc_common_config.h"
#include "udp/udp_server.h"
#include "wifi/wifi.h"
#include "battery_detect/bat_adc.h"
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
    xTaskCreate(ADC_oneshot_Task,"ADC_Task",4096,NULL,5,NULL);
    // xTaskCreate(BAT_detect_Task,"BAT_detect_Task",4096,NULL,2,NULL);


    ret = wifi_sta_init();
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG,"ESP WIFI init failed!");
    }
    espnow_init();
    
    

    
}
