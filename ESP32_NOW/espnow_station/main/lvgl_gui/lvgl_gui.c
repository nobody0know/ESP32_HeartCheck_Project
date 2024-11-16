#include "lvgl__lvgl/lvgl.h"
#include "lvgl_gui/generated/gui_guider.h"
#include "lvgl_gui/custom/custom.h"
#include "generated/events_init.h"
#include "guider_customer_fonts.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "event_group_config.h"

static const char *TAG = "LVGL";
lv_ui guider_ui;
EventGroupHandle_t gui_event_group;

void lvgl_gui_esptouch_screen(lv_ui *ui)
{
    setup_scr_esptouch_screen(ui);
    lv_scr_load(ui->esptouch_screen);
}

void lvgl_gui_main_screen(lv_ui *ui)
{
    setup_scr_main_screen(ui);
    lv_scr_load(ui->main_screen);
}

void lvgl_gui_update()
{
    char text_show_buf[200] = {0};
    extern uint8_t ssid[33];
    sprintf(text_show_buf, "" LV_SYMBOL_WIFI " 连接的wifi为\"%s\"", (char *)ssid);
    lv_label_set_text(guider_ui.main_screen_label_1, text_show_buf);

    // memset(text_show_buf,0,sizeof(text_show_buf));
    // char * ip_addr;
    // sprintf(text_show_buf,"" LV_SYMBOL_HOME " 主机IP地址为\"%s\"",ip_addr);
    // lv_label_set_text(guider_ui.main_screen_label_3, "");

    memset(text_show_buf, 0, sizeof(text_show_buf));
    extern uint8_t total_nodedevices_num;
    sprintf(text_show_buf, "" LV_SYMBOL_GPS " 已配网节点数：%d个", total_nodedevices_num);
    // ESP_LOGI(TAG,"已配网节点数：%d个",total_nodedevices_num);
    lv_label_set_text(guider_ui.main_screen_label_2, text_show_buf);
}

void lvgl_task()
{
    xEventGroupWaitBits(gui_event_group, LCD_INIT_OK_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    lvgl_gui_esptouch_screen(&guider_ui);
    xEventGroupWaitBits(gui_event_group, LCD_WIFI_OK_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    lvgl_gui_main_screen(&guider_ui);
    lv_timer_create(lvgl_gui_update, 200, NULL);
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void lvgl_gui_init()
{
    gui_event_group = xEventGroupCreate();
    xTaskCreate(lvgl_task, "lvgl_task", 8192, NULL, 3, NULL);
}
