set(srcs 
        "main.c"
        "st7789.c"
        "fontx.c"
   )

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS ".")
