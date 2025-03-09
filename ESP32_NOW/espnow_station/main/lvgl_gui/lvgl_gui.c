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
#include "wifi_config.h"
#include "udp_app.h"

static const char *TAG = "LVGL";
lv_ui guider_ui;
EventGroupHandle_t gui_event_group;
uint8_t event_now = 0;

void lvgl_gui_esptouch_screen(lv_ui *ui)
{
    setup_scr_esptouch_screen(ui);
    lv_scr_load(ui->esptouch_screen);
}

void lvgl_wifi_connect_screen(lv_ui *ui)
{
    setup_scr_wifi_connect_screen(ui);
    lv_scr_load(ui->wifi_connect_screen);
}

void lvgl_gui_main_screen(lv_ui *ui)
{
    setup_scr_main_screen(ui);
    lv_scr_load(ui->main_screen);
}

void lvgl_gui_update()
{
    switch (event_now)
    {
    case LCD_INIT_OK_BIT:

        break;
    case LCD_WIFI_OK_BIT:
        lv_label_set_text(guider_ui.wifi_connect_screen_wifi_connect_log, "wifi连接成功");
        lv_label_set_text_fmt(guider_ui.wifi_connect_screen_pc_connect_log, "基站IP地址为:\"%s\"", get_ip_addr());
        break;
    case LCD_GET_PCIP_OK_BIT:
        lv_label_set_text(guider_ui.wifi_connect_screen_prov_log, "采集程序连接完成");
        break;
    case LCD_UPDATE_MAIN_BIT:
    {
        char text_show_buf[200] = {0};
        extern char wifi_ssid[33];
        sprintf(text_show_buf, "" LV_SYMBOL_WIFI " 连接的wifi为\"%s\"", (char *)wifi_ssid);
        lv_label_set_text(guider_ui.main_screen_label_1, text_show_buf);

        memset(text_show_buf, 0, sizeof(text_show_buf));
        extern uint8_t total_nodedevices_num;
        sprintf(text_show_buf, "" LV_SYMBOL_GPS " 已配网节点数：%d个", total_nodedevices_num);

        // ESP_LOGI(TAG, "已配网节点数：%d个", total_nodedevices_num);
        lv_label_set_text(guider_ui.main_screen_label_2, text_show_buf);

        lv_label_set_text_fmt(guider_ui.main_screen_label_3, "" LV_SYMBOL_HOME " 主机IP地址为\" %s\"",get_pc_ip_addr());
    }
    break;

    default:
        ESP_LOGE(TAG, "GUI stateflow have some problem!");
        break;
    }
}

void lvgl_task()
{
    xEventGroupWaitBits(gui_event_group, LCD_INIT_OK_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    lv_timer_create(lvgl_gui_update, 200, NULL);
    event_now = LCD_INIT_OK_BIT;
    if (ifneed_wifi_provision() == 0)
    {
        lvgl_wifi_connect_screen(&guider_ui);
    }
    else
    {
        lvgl_gui_esptouch_screen(&guider_ui);
    }

    xEventGroupWaitBits(gui_event_group, LCD_WIFI_OK_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    event_now = LCD_WIFI_OK_BIT;
    lvgl_wifi_connect_screen(&guider_ui);

    xEventGroupWaitBits(gui_event_group, LCD_GET_PCIP_OK_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    event_now = LCD_GET_PCIP_OK_BIT;

    xEventGroupWaitBits(gui_event_group, LCD_UPDATE_MAIN_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    event_now = LCD_UPDATE_MAIN_BIT;
    lvgl_gui_main_screen(&guider_ui);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void lvgl_gui_init()
{
    gui_event_group = xEventGroupCreate();
    xTaskCreate(lvgl_task, "lvgl_task", 2048, NULL, 3, NULL);
}

void show_esptouch_screen()
{
    lvgl_gui_esptouch_screen(&guider_ui);
}
