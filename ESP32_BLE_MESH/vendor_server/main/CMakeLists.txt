set(srcs "main.c"
        "board.c")

idf_component_register(SRCS "WS2812/WS2812.c" "ble_mesh/ble_mesh.c" "${srcs}"
                    INCLUDE_DIRS  "." "WS2812" "ble_mesh")
