/* board.c - Board-specific hooks */

/*
 * SPDX-FileCopyrightText: 2017 Intel Corporation
 * SPDX-FileContributor: 2018-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "driver/gpio.h"
#include "button/include/iot_button.h"
#include "esp_log.h"
#include "board.h"

#define TAG "BOARD"

#define BUTTON_IO_NUM           9
#define BUTTON_ACTIVE_LEVEL     0

bool usermsg_send_start= 0;

static void button_tap_cb(void* arg)
{
    if(usermsg_send_start == 0)
    {
        ESP_LOGI(TAG, "START SEND...");
        usermsg_send_start = 1;
    }
    else
    {
        ESP_LOGI(TAG, "END SEND...");
        usermsg_send_start = 0;
    }
}

static void board_button_init(void)
{
    button_handle_t btn_handle = iot_button_create(BUTTON_IO_NUM, BUTTON_ACTIVE_LEVEL);
    if (btn_handle) {
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_RELEASE, button_tap_cb, "RELEASE");
    }
}

void board_init(void)
{
    board_button_init();
}