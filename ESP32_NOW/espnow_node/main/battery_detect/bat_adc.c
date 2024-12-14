/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

const static char *TAG = "BAT-ADC";

// ADC1 Channels
#define BAT_ADC1_CHAN4 ADC_CHANNEL_4
#define BAT_ADC_ATTEN ADC_ATTEN_DB_12

static int adc_raw[1][10];
static int voltage[1][10];

extern QueueHandle_t ADC_queue;
extern uint32_t time_flag_gap;

// ADC Calibration

static bool battery_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

void BAT_adc_init(adc_oneshot_unit_handle_t adc1_handle, adc_oneshot_chan_cfg_t *config)
{
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, BAT_ADC1_CHAN4, config));
}

void BAT_detect_Task(void *pvParameter)
{
    extern adc_oneshot_unit_handle_t adc1_handle;
    // ADC1 Calibration Init
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = battery_adc_calibration_init(ADC_UNIT_1, BAT_ADC1_CHAN4, BAT_ADC_ATTEN, &adc1_cali_chan0_handle);

    while (1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, BAT_ADC1_CHAN4, &adc_raw[0][0]));
        // ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, BAT_ADC1_CHAN4, adc_raw[0][0]);
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0][0], &voltage[0][0]));
        // ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, BAT_ADC1_CHAN4, voltage[0][0]);
        // ESP_LOGI(TAG, "BAT_ADC: %d mV", voltage[0][0] * 2);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
