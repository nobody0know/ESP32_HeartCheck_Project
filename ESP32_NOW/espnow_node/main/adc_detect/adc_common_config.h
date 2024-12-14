/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define ECG_ADC_UNIT                    ADC_UNIT_1
#define _ECG_ADC_UNIT_STR(unit)         #unit
#define ECG_ADC_UNIT_STR(unit)          _ECG_ADC_UNIT_STR(unit)
#define ECG_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define ECG_ADC_ATTEN                   ADC_ATTEN_DB_12
#define ECG_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define ECG_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define ECG_ADC_GET_CHANNEL(p_data)     ((p_data)->type1.channel)
#define ECG_ADC_GET_DATA(p_data)        ((p_data)->type1.data)
#else
#define ECG_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define ECG_ADC_GET_CHANNEL(p_data)     ((p_data)->type2.channel)
#define ECG_ADC_GET_DATA(p_data)        ((p_data)->type2.data)
#endif

//continuous used
#define ECG_READ_LEN                    256

extern void ADC_oneshot_Task(void *pvParameter);
extern void ADC_continuous_Task(void *pvParameter);