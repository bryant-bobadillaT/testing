idf_component_register(
    SRCS "c4001_driver.cpp" "c4001_service.cpp"
    INCLUDE_DIRS "include"
    # "driver/uart.h" lives in esp_driver_uart, so propagate that requirement
    # along with the shared driver and FreeRTOS helpers to every consumer.
    REQUIRES esp_driver_uart driver freertos
)
