set(srcs "main.c"
        "board.c")

idf_component_register(SRCS "WS2812.c" "${srcs}"
                    INCLUDE_DIRS  ".")
