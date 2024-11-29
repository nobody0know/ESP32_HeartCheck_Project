file(GLOB_RECURSE srcs *.c
                    ./main_init.c 
                    esp_now_app/*.c 
                    lcd/*.c
                    wifi_config/*.c
                    lvgl_gui/*.c
                    lvgl_gui/generated/*.c
                    lvgl_gui/generated/guider_fonts/*.c
                    lvgl_gui/custom/*.c
                    lvgl_gui/generated/images/*.c
                    lvgl_gui/generated/guider_customer_fonts/*.c
                    udp/*.c
                    )


set(include_dirs 
.  
esp_now_app
lcd
wifi_config
udp
lvgl_gui 
lvgl_gui/custom
lvgl_gui/generated/guider_fonts
lvgl_gui/generated/guider_customer_fonts
lvgl_gui/generated/
lvgl_gui/generated/images
)

idf_component_register(SRCS ${srcs} 
                    INCLUDE_DIRS ${include_dirs} )
# idf_component_register(SRCS "espnow_example_main.c" "smart_config/smart_config.c" "esp_now_app/esp_now_app.c" "lcd/lcd_hw.c" "lcd/lvgl_gui_demo_ui.c"
#                     INCLUDE_DIRS "." "esp_now_app" "smart_config" "lcd")
