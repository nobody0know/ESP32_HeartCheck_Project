idf_component_register(SRCS "espnow_example_main.c" "board.c" "ws2812/ws2812.c" "esp_now/esp_now_app.c" "adc_detect/oneshot_detect.c" ""adc_detect/continuous_detect.c"" "udp/udp_server.c" "wifi/wifi.c"
                    INCLUDE_DIRS "." "ws2812" "esp_now" "adc_detect" "udp" "wifi"
                    )
