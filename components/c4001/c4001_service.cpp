#include "c4001/c4001_service.hpp"

#include "esp_check.h"
#include "esp_log.h"

namespace c4001 {
namespace {
constexpr const char *kTag = "c4001_service";
}

Service::Service() : task_(nullptr), queue_(nullptr), poll_period_(pdMS_TO_TICKS(500)) {}

Service::~Service() { stop(); }

esp_err_t Service::start(const UartConfig &config, TickType_t poll_period, size_t queue_depth) {
  if (task_) {
    return ESP_ERR_INVALID_STATE;
  }

  ESP_RETURN_ON_ERROR(driver_.init(config), kTag, "driver init failed");

  queue_ = xQueueCreate(queue_depth, sizeof(Event));
  if (!queue_) {
    return ESP_ERR_NO_MEM;
  }

  poll_period_ = poll_period;

  BaseType_t ok = xTaskCreate(Service::taskTrampoline, "c4001_task", 4096, this, 5, &task_);
  if (ok != pdPASS) {
    vQueueDelete(queue_);
    queue_ = nullptr;
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}

void Service::stop() {
  if (task_) {
    TaskHandle_t task = task_;
    task_ = nullptr;
    vTaskDelete(task);
  }
  if (queue_) {
    vQueueDelete(queue_);
    queue_ = nullptr;
  }
}

void Service::taskTrampoline(void *ctx) {
  static_cast<Service *>(ctx)->run();
}

void Service::run() {
  TickType_t delay = poll_period_ ? poll_period_ : pdMS_TO_TICKS(500);
  while (true) {
    PresenceReport presence;
    if (driver_.queryPresence(&presence, delay) == ESP_OK) {
      Event evt = {
          .type = EventType::kPresence,
          .data = {.presence = {.sample = {xTaskGetTickCount(), presence}}},
      };
      if (queue_) {
        xQueueSend(queue_, &evt, 0);
      }
    }

    HeartbeatReport heartbeat;
    if (driver_.queryHeartbeat(&heartbeat, delay) == ESP_OK) {
      Event evt = {
          .type = EventType::kHeartbeat,
          .data = {.heartbeat = {.sample = {xTaskGetTickCount(), heartbeat}}},
      };
      if (queue_) {
        xQueueSend(queue_, &evt, 0);
      }
    }

    vTaskDelay(delay);
  }
}

}  // namespace c4001
