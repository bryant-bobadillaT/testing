#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "c4001/c4001_driver.hpp"
#include "esp_err.h"

namespace c4001 {

enum class EventType { kPresence, kHeartbeat };

struct PresenceEvent {
  PresenceSample sample;
};

struct HeartbeatEvent {
  HeartbeatSample sample;
};

struct Event {
  EventType type;
  union {
    PresenceEvent presence;
    HeartbeatEvent heartbeat;
  } data;
};

class Service {
 public:
  Service();
  ~Service();

  esp_err_t start(const UartConfig &config, TickType_t poll_period = pdMS_TO_TICKS(500), size_t queue_depth = 8);
  void stop();

  QueueHandle_t event_queue() const { return queue_; }

 private:
  static void taskTrampoline(void *ctx);
  void run();

  Driver driver_;
  TaskHandle_t task_;
  QueueHandle_t queue_;
  TickType_t poll_period_;
};

}  // namespace c4001
