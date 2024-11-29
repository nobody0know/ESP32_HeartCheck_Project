/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_wifi_connect_screen(lv_ui *ui)
{
    //Write codes wifi_connect_screen
    ui->wifi_connect_screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->wifi_connect_screen, 320, 240);
    lv_obj_set_scrollbar_mode(ui->wifi_connect_screen, LV_SCROLLBAR_MODE_OFF);

    //Write style for wifi_connect_screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->wifi_connect_screen, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->wifi_connect_screen, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->wifi_connect_screen, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_connect_screen_pc_connect_log
    ui->wifi_connect_screen_pc_connect_log = lv_label_create(ui->wifi_connect_screen);
    lv_label_set_text(ui->wifi_connect_screen_pc_connect_log, "基站IP地址为:\"\"");
    lv_label_set_long_mode(ui->wifi_connect_screen_pc_connect_log, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->wifi_connect_screen_pc_connect_log, 35, 91);
    lv_obj_set_size(ui->wifi_connect_screen_pc_connect_log, 252, 32);

    //Write style for wifi_connect_screen_pc_connect_log, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_connect_screen_pc_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_connect_screen_pc_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_connect_screen_pc_connect_log, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_connect_screen_pc_connect_log, &lv_font_vivo_Sans_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_connect_screen_pc_connect_log, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->wifi_connect_screen_pc_connect_log, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->wifi_connect_screen_pc_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_connect_screen_pc_connect_log, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_connect_screen_pc_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_connect_screen_pc_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_connect_screen_pc_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_connect_screen_pc_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_connect_screen_pc_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_connect_screen_pc_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_connect_screen_wifi_connect_log
    ui->wifi_connect_screen_wifi_connect_log = lv_label_create(ui->wifi_connect_screen);
    lv_label_set_text(ui->wifi_connect_screen_wifi_connect_log, "正在连接wifi......");
    lv_label_set_long_mode(ui->wifi_connect_screen_wifi_connect_log, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->wifi_connect_screen_wifi_connect_log, 75, 33);
    lv_obj_set_size(ui->wifi_connect_screen_wifi_connect_log, 173, 32);

    //Write style for wifi_connect_screen_wifi_connect_log, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_connect_screen_wifi_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_connect_screen_wifi_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_connect_screen_wifi_connect_log, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_connect_screen_wifi_connect_log, &lv_font_vivo_Sans_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_connect_screen_wifi_connect_log, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->wifi_connect_screen_wifi_connect_log, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->wifi_connect_screen_wifi_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_connect_screen_wifi_connect_log, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_connect_screen_wifi_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_connect_screen_wifi_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_connect_screen_wifi_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_connect_screen_wifi_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_connect_screen_wifi_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_connect_screen_wifi_connect_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes wifi_connect_screen_prov_log
    ui->wifi_connect_screen_prov_log = lv_label_create(ui->wifi_connect_screen);
    lv_label_set_text(ui->wifi_connect_screen_prov_log, "等待采集程序连接......\n");
    lv_label_set_long_mode(ui->wifi_connect_screen_prov_log, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->wifi_connect_screen_prov_log, 64, 149);
    lv_obj_set_size(ui->wifi_connect_screen_prov_log, 195, 32);

    //Write style for wifi_connect_screen_prov_log, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->wifi_connect_screen_prov_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->wifi_connect_screen_prov_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->wifi_connect_screen_prov_log, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->wifi_connect_screen_prov_log, &lv_font_vivo_Sans_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->wifi_connect_screen_prov_log, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->wifi_connect_screen_prov_log, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->wifi_connect_screen_prov_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->wifi_connect_screen_prov_log, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->wifi_connect_screen_prov_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->wifi_connect_screen_prov_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->wifi_connect_screen_prov_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->wifi_connect_screen_prov_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->wifi_connect_screen_prov_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->wifi_connect_screen_prov_log, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of wifi_connect_screen.


    //Update current screen layout.
    lv_obj_update_layout(ui->wifi_connect_screen);

}
