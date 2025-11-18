# ESP32-C5 C4001 mmWave service

This repository now exposes a reusable ESP-IDF component that wraps the DFRobot C4001 mmWave presence/heartbeat sensor. The `c4001` component replaces the Arduino-only APIs with the ESP-IDF UART driver and FreeRTOS tasks.

## Component overview

* `c4001_protocol.hpp` defines the binary frame layout used by the module (`0x53 0x59` header, `0x54 0x43` trailer) and the known command IDs for presence and heartbeat queries. 【F:components/c4001/include/c4001/c4001_protocol.hpp†L7-L38】
* `c4001_driver.hpp/.cpp` implements a synchronous UART transport on top of `uart_param_config`, `uart_driver_install`, `uart_write_bytes`, and `uart_read_bytes`. The driver can send raw commands or directly request parsed presence/heartbeat measurements. 【F:components/c4001/c4001_driver.cpp†L17-L126】
* `c4001_service.hpp/.cpp` hosts the FreeRTOS polling task. It replaces the busy `delay()` loops from Arduino with a queue-driven task that periodically issues presence/heartbeat queries and pushes typed events to an application queue. 【F:components/c4001/c4001_service.cpp†L5-L68】

## FreeRTOS task

The service task alternates between presence and heartbeat queries, then sleeps for the configured polling interval with `vTaskDelay`. Each successful frame is converted into a `c4001::Event` and delivered through the queue returned by `Service::event_queue()`. 【F:components/c4001/c4001_service.cpp†L39-L68】

## Example application

`main/main.cpp` mirrors the original Arduino example by:

1. Configuring UART1 (GPIO2/3) at 115200 baud.
2. Starting the `c4001::Service` with a 500 ms poll period.
3. Spawning a logger task that consumes events from the queue and prints presence/heartbeat updates.

This approach validates the driver without busy polling and is ready to run on an ESP32-C5 devkit via `idf.py set-target esp32c5 && idf.py flash monitor`. 【F:main/main.cpp†L3-L53】【F:main/main.cpp†L55-L76】

## Usage snippet

```cpp
c4001::UartConfig cfg;
cfg.port = UART_NUM_1;
cfg.tx_io_num = 2;
cfg.rx_io_num = 3;

c4001::Service service;
ESP_ERROR_CHECK(service.start(cfg, pdMS_TO_TICKS(250), 8));

c4001::Event event;
while (xQueueReceive(service.event_queue(), &event, pdMS_TO_TICKS(1000)) == pdTRUE) {
  // handle presence / heartbeat events
}
```

Refer to `docs/DFRobot_C4001_dependency_analysis.md` for the full list of Arduino concepts that were replaced during the porting effort.

## Hardware hookup

For the ESP32-C5 devkits, GPIO2 (I2C SDA) and GPIO3 (I2C SCL) are routed to UART1's TX/RX pads, so the sensor wiring mirrors the default configuration used by `main/main.cpp`:

* C4001 **TX** → ESP32-C5 **GPIO3** (UART1 RX / I2C SCL)
* C4001 **RX** → ESP32-C5 **GPIO2** (UART1 TX / I2C SDA)
* C4001 **VIN** → ESP32-C5 5 V (or 3.3 V if your breakout supports it)
* C4001 **GND** → ESP32-C5 GND

Hardware flow control is disabled, so RTS/CTS remain unconnected.
