#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_check.h"
#include "esp_err.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

static const char *TAG = "GPIO";

#define I2C_SCL_IO      (GPIO_NUM_1)
#define I2C_SDA_IO      (GPIO_NUM_0)
#define I2C_NUM         (0)

esp_err_t gpio_init()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE, 
        .mode = GPIO_MODE_OUTPUT,       
        .pin_bit_mask = 1 << GPIO_NUM_5,       
        .pull_down_en = 0,              
        .pull_up_en = 1,                
    };
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_5, 1);

    // /* Initialize I2C peripheral */
    // const i2c_config_t es_i2c_cfg = {
    //     .sda_io_num = I2C_SDA_IO,
    //     .scl_io_num = I2C_SCL_IO,
    //     .mode = I2C_MODE_MASTER,
    //     .sda_pullup_en = GPIO_PULLUP_ENABLE,
    //     .scl_pullup_en = GPIO_PULLUP_ENABLE,
    //     .master.clk_speed = 100000,
    // };
    // ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM, &es_i2c_cfg), TAG, "config i2c failed");
    // ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0), TAG, "install i2c driver failed");
    return ESP_OK;
}