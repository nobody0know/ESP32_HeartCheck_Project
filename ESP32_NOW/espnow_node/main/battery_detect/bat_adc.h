#include "esp_adc/adc_oneshot.h"
void BAT_detect_Task(void *pvParameter);
void BAT_adc_init(adc_oneshot_unit_handle_t adc1_handle,adc_oneshot_chan_cfg_t *config);