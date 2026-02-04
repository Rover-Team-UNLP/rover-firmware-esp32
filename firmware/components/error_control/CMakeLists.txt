idf_component_register(
    SRCS "src/error_control.c"
    INCLUDE_DIRS 
        "include"
        "../../include"
    REQUIRES 
        freertos 
        esp_common 
        esp_timer
        log
)
