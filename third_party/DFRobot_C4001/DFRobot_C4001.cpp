#include "DFRobot_C4001.h"

#include <string.h>

static const uint8_t kFrameHeader[] = {0x53, 0x59};
static const uint8_t kFrameTail[] = {0x54, 0x43};
static const uint8_t kPresenceCmd = 0x80;
static const uint8_t kHeartbeatCmd = 0x81;

DFRobot_C4001 *DFRobot_C4001::s_activeInstance = nullptr;

DFRobot_C4001::DFRobot_C4001(Stream &stream, uint8_t irqPin)
    : _stream(&stream), _irqPin(irqPin), _interruptFlag(false) {}

DFRobot_C4001::~DFRobot_C4001() {
  if (s_activeInstance == this) {
    detachInterrupt(digitalPinToInterrupt(_irqPin));
    s_activeInstance = nullptr;
  }
}

bool DFRobot_C4001::begin(uint32_t baud) {
  if (!_stream) {
    return false;
  }
  ((HardwareSerial *)_stream)->begin(baud);
  flushInput();

  pinMode(_irqPin, INPUT_PULLUP);
  s_activeInstance = this;
  attachInterrupt(digitalPinToInterrupt(_irqPin), DFRobot_C4001::isrRouter, FALLING);
  delay(20);
  return true;
}

bool DFRobot_C4001::available() {
  if (_interruptFlag) {
    noInterrupts();
    _interruptFlag = false;
    interrupts();
    return true;
  }
  return false;
}

bool DFRobot_C4001::readPresence(c4001_presence_info_t &out) {
  uint8_t payload[8];
  if (!readFrame(payload, sizeof(payload))) {
    return false;
  }
  if (payload[0] != kPresenceCmd) {
    return false;
  }
  out.humanDetected = payload[1];
  out.motionDetected = payload[2];
  out.distanceCm = (payload[3] << 8) | payload[4];
  return true;
}

bool DFRobot_C4001::readHeartbeat(c4001_heartbeat_info_t &out) {
  uint8_t payload[8];
  if (!readFrame(payload, sizeof(payload))) {
    return false;
  }
  if (payload[0] != kHeartbeatCmd) {
    return false;
  }
  out.heartbeatDetected = payload[1];
  out.bpm = (payload[2] << 8) | payload[3];
  out.confidence = payload[4];
  return true;
}

void DFRobot_C4001::clear() {
  flushInput();
}

void DFRobot_C4001::flushInput() {
  while (_stream && _stream->available()) {
    _stream->read();
    delay(2);
  }
}

bool DFRobot_C4001::readFrame(uint8_t *payload, size_t payloadSize) {
  if (!_stream) {
    return false;
  }
  uint8_t buffer[16];
  size_t offset = 0;
  unsigned long start = millis();
  while (offset < sizeof(buffer)) {
    if (_stream->available()) {
      buffer[offset++] = _stream->read();
    } else if (millis() - start > 20) {
      return false;
    }
  }
  if (memcmp(buffer, kFrameHeader, sizeof(kFrameHeader)) != 0) {
    return false;
  }
  if (memcmp(buffer + sizeof(buffer) - sizeof(kFrameTail), kFrameTail, sizeof(kFrameTail)) != 0) {
    return false;
  }
  memcpy(payload, buffer + 3, min(payloadSize, (size_t)8));
  return true;
}

void DFRobot_C4001::isrRouter() {
  if (s_activeInstance) {
    s_activeInstance->onInterrupt();
  }
}

void DFRobot_C4001::onInterrupt() {
  _interruptFlag = true;
}
