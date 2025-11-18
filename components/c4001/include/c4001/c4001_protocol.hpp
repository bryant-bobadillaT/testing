#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace c4001 {

constexpr uint8_t kFrameHeader0 = 0x53;
constexpr uint8_t kFrameHeader1 = 0x59;
constexpr uint8_t kFrameTail0 = 0x54;
constexpr uint8_t kFrameTail1 = 0x43;
constexpr size_t kMaxPayloadSize = 32;
constexpr size_t kFrameOverhead = 2 /*header*/ + 1 /*len*/ + 1 /*cmd*/ + 1 /*checksum*/ + 2 /*tail*/;
constexpr size_t kFrameSize = kFrameOverhead + kMaxPayloadSize;

enum class Command : uint8_t {
  kQueryPresence = 0x80,
  kQueryHeartbeat = 0x81,
  kSetDistanceGate = 0x01,
  kSetReportRate = 0x02,
};

enum class PresenceState : uint8_t {
  kNoTarget = 0,
  kMoving = 1,
  kStationary = 2,
};

struct PresenceReport {
  PresenceState state;
  uint16_t distance_cm;
};

struct HeartbeatReport {
  bool human_present;
  uint16_t bpm;
  uint8_t confidence;
};

struct Frame {
  uint8_t length;
  Command command;
  std::array<uint8_t, kMaxPayloadSize> payload;
  uint8_t checksum;
};

inline uint8_t calc_checksum(const uint8_t *data, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 0; i < len; ++i) {
    sum += data[i];
  }
  return sum;
}

}  // namespace c4001
