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



void setup_scr_esptouch_screen(lv_ui *ui)
{
    //Write codes esptouch_screen
    ui->esptouch_screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->esptouch_screen, 320, 240);
    lv_obj_set_scrollbar_mode(ui->esptouch_screen, LV_SCROLLBAR_MODE_OFF);

    //Write style for esptouch_screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->esptouch_screen, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->esptouch_screen, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->esptouch_screen, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes esptouch_screen_label_1
    ui->esptouch_screen_label_1 = lv_label_create(ui->esptouch_screen);
    lv_label_set_text(ui->esptouch_screen_label_1, "等待ESPtouch配网......");
    lv_label_set_long_mode(ui->esptouch_screen_label_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->esptouch_screen_label_1, 42, 183);
    lv_obj_set_size(ui->esptouch_screen_label_1, 227, 32);

    //Write style for esptouch_screen_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->esptouch_screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->esptouch_screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->esptouch_screen_label_1, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->esptouch_screen_label_1, &lv_font_vivo_Sans_16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->esptouch_screen_label_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->esptouch_screen_label_1, 1, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->esptouch_screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->esptouch_screen_label_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->esptouch_screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->esptouch_screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->esptouch_screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->esptouch_screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->esptouch_screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->esptouch_screen_label_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes esptouch_screen_img_1
    ui->esptouch_screen_img_1 = lv_img_create(ui->esptouch_screen);
    lv_obj_add_flag(ui->esptouch_screen_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(ui->esptouch_screen_img_1, &_esptouch_alpha_150x150);
    lv_img_set_pivot(ui->esptouch_screen_img_1, 50,50);
    lv_img_set_angle(ui->esptouch_screen_img_1, 0);
    lv_obj_set_pos(ui->esptouch_screen_img_1, 81, 12);
    lv_obj_set_size(ui->esptouch_screen_img_1, 150, 150);

    //Write style for esptouch_screen_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->esptouch_screen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->esptouch_screen_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->esptouch_screen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->esptouch_screen_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of esptouch_screen.


    //Update current screen layout.
    lv_obj_update_layout(ui->esptouch_screen);

}
