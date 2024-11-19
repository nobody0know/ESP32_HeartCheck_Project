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

