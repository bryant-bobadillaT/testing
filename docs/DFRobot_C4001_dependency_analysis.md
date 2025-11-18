# DFRobot_C4001 Arduino dependency inventory

The upstream Arduino driver (`DFRobot_C4001.cpp`) tightly couples the mmWave radar to the Wiring/Arduino runtime. The following table highlights each dependency we need to account for when porting to ESP-IDF.

| Concern | Arduino usage | Notes |
| --- | --- | --- |
| Serial I/O | The driver stores a `Stream*` and assumes the object is a `HardwareSerial`. All reads and writes are performed through that pointer (`_stream->available()`, `_stream->read()`, `((HardwareSerial*)_stream)->begin(baud)`). 【F:third_party/DFRobot_C4001/DFRobot_C4001.cpp†L11-L39】【F:third_party/DFRobot_C4001/DFRobot_C4001.cpp†L64-L87】 |
| Timing | The module relies on blocking `delay()` inside `begin()` and while flushing the serial buffer. Timeouts inside `readFrame()` are implemented with `millis()` polling. 【F:third_party/DFRobot_C4001/DFRobot_C4001.cpp†L25-L39】【F:third_party/DFRobot_C4001/DFRobot_C4001.cpp†L60-L83】 |
| Interrupts | The library configures a GPIO IRQ pin and uses `attachInterrupt`/`detachInterrupt` along with global `noInterrupts()`/`interrupts()` guards to signal new data frames. 【F:third_party/DFRobot_C4001/DFRobot_C4001.cpp†L15-L57】 |

These behaviors guide the ESP-IDF port: we replace `Stream` with the UART driver, re-map `delay()`/`millis()` to FreeRTOS-aware calls, and model the interrupt notification with FreeRTOS queues.
