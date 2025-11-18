#pragma once

#include <Arduino.h>
#include <stdint.h>

typedef struct {
  bool humanDetected;
  bool motionDetected;
  uint16_t distanceCm;
} c4001_presence_info_t;

typedef struct {
  bool heartbeatDetected;
  uint16_t bpm;
  uint8_t confidence;
} c4001_heartbeat_info_t;

class DFRobot_C4001 {
 public:
  explicit DFRobot_C4001(Stream &stream, uint8_t irqPin);
  ~DFRobot_C4001();

  bool begin(uint32_t baud = 115200);
  bool available();

  bool readPresence(c4001_presence_info_t &out);
  bool readHeartbeat(c4001_heartbeat_info_t &out);

  void clear();

 private:
  void flushInput();
  bool readFrame(uint8_t *payload, size_t payloadSize);

  static void isrRouter();
  void onInterrupt();

  static DFRobot_C4001 *s_activeInstance;

  Stream *_stream;
  uint8_t _irqPin;
  volatile bool _interruptFlag;
};
