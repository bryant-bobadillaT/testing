# ESP32-C5 C4001 mmWave playground

This repo hosts an ESP-IDF workspace that ports the Arduino-only [DFRobot C4001](https://www.dfrobot.com/product-2521.html) mmWave presence/heartbeat sensor to the ESP32-C5.

## Layout

```
components/c4001/    -> reusable driver/service component
main/                -> minimal FreeRTOS app that logs presence + heartbeat events
third_party/         -> trimmed Arduino source used for dependency analysis
```

## Getting started

```
. $IDF_PATH/export.sh
idf.py set-target esp32c5
idf.py build flash monitor
```

The `c4001::Service` encapsulates the polling logic and uses event queues rather than busy loops, so the application remains responsive while logging mmWave presence and heartbeat updates. Check `docs/mmwave_service.md` for architecture details.

## Wiring the mmWave sensor

The ESP32-C5 example assumes the mmWave module is connected to UART1 on GPIO2/GPIO3, which are labeled as I2C SDA/SCL on most dev kits. Wire it as follows:

1. Power the C4001 from the ESP32-C5's 5 V (or regulated 3.3 V, depending on your module revision) and GND rails.
2. Connect the sensor's **TX** pin to ESP32-C5 **GPIO3** (UART1 RX / I2C SCL).
3. Connect the sensor's **RX** pin to ESP32-C5 **GPIO2** (UART1 TX / I2C SDA).
4. Leave RTS/CTS unconnected; the driver uses 3-wire UART without hardware flow control.

With this wiring in place, the out-of-the-box configuration in `main/main.cpp` will match your hardware and no additional code changes are required.
