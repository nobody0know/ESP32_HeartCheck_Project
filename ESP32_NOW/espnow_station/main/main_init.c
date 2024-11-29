/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_now_app.h"
#include "wifi_config.h"
#include "lcd_hw.h"
#include "lvgl_gui.h"
#include "udp_app.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t s_wifi_event_group;

uint8_t reset_flag = 0;

void app_main(void)
{
    lcd_hw_init();
    lvgl_gui_init();
    
    wifi_config_init();

    udp_app_init();

    ESP_ERROR_CHECK(espnow_init());

    reset_flag = 1;
}
