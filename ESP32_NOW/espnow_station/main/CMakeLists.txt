idf_component_register(SRCS "espnow_example_main.c" "smart_config/smart_config.c" "esp_now_app/esp_now_app.c" "lcd/lcd_hw.c" "lcd/lvgl_demo_ui.c"
                    INCLUDE_DIRS "." "esp_now_app" "smart_config" "lcd")
