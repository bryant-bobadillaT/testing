idf_component_register(SRCS "c4001_driver.cpp" "c4001_service.cpp"
                      INCLUDE_DIRS "include"
                      REQUIRES driver esp_driver_uart freertos)
