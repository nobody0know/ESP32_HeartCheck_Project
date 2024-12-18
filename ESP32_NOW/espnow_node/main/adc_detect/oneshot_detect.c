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
#include "esp_now/esp_now_app.h"
#include "adc_detect/adc_common_config.h"
#include "sdkconfig.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_timer.h"

#include "bat_adc.h"

const static char *TAG = "ADC";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
// ADC1 Channels
#define ECG_ADC1_CHAN0 ADC_CHANNEL_0

adc_oneshot_unit_handle_t adc1_handle;
static int adc_raw[1][10];
static int voltage[1][10];

extern QueueHandle_t ADC_queue;
extern uint32_t time_flag_gap;

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool ecg_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
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
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

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

static void ecg_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

void ADC_oneshot_Task(void *pvParameter)
{
    //-------------ADC1 Init---------------//

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ECG_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ECG_ADC1_CHAN0, &config));

    BAT_adc_init(adc1_handle, &config);

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = ecg_adc_calibration_init(ADC_UNIT_1, ECG_ADC1_CHAN0, ECG_ADC_ATTEN, &adc1_cali_chan0_handle);

    while (1)
    {
        extern bool usermsg_send_start;
        if (usermsg_send_start == 1 && do_calibration1_chan0)
        {
            // uint32_t in_tick = esp_timer_get_time();
            payload_msg adc_true_value;
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ECG_ADC1_CHAN0, &adc_raw[0][0]));
            // ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ECG_ADC1_CHAN0, adc_raw[0][0]);

            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0][0], &voltage[0][0]));
            // ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, ECG_ADC1_CHAN0, voltage[0][0]);
            adc_true_value.payload_data.adc_value = (uint16_t)voltage[0][0];
            adc_true_value.payload_data.timestamp = esp_log_timestamp() + time_flag_gap;
            // ESP_LOGI(TAG, "time stamp %ld adc value: %d", adc_true_value.payload_data.timestamp, adc_true_value.payload_data.adc_value);
            xQueueSend(ADC_queue, &adc_true_value, 10);

            // uint32_t out_tick = esp_timer_get_time();
            // uint32_t time_diff = out_tick - in_tick;
            // ESP_LOGI(TAG, "time diff: %ld us", time_diff);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    if (do_calibration1_chan0)
    {
        ecg_adc_calibration_deinit(adc1_cali_chan0_handle);
    }
}
