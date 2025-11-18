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
