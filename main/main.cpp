#include <inttypes.h>

#include "c4001/c4001_service.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr const char *kTag = "c4001_app";

void event_consumer(void *ctx) {
  c4001::Service *service = static_cast<c4001::Service *>(ctx);
  c4001::Event event;
  QueueHandle_t queue = service->event_queue();
  while (queue && xQueueReceive(queue, &event, portMAX_DELAY) == pdTRUE) {
    switch (event.type) {
      case c4001::EventType::kPresence: {
        const auto &sample = event.data.presence.sample;
        const char *state = "none";
        switch (sample.report.state) {
          case c4001::PresenceState::kNoTarget:
            state = "none";
            break;
          case c4001::PresenceState::kMoving:
            state = "moving";
            break;
          case c4001::PresenceState::kStationary:
            state = "stationary";
            break;
        }
        ESP_LOGI(kTag, "Presence: %s at %ucm (tick=%" PRIu32 ")", state, sample.report.distance_cm, sample.timestamp);
        break;
      }
      case c4001::EventType::kHeartbeat: {
        const auto &sample = event.data.heartbeat.sample;
        ESP_LOGI(kTag, "Heartbeat: %ubpm confidence=%u (tick=%" PRIu32 ")", sample.report.bpm,
                 sample.report.confidence, sample.timestamp);
        break;
      }
    }
  }
  vTaskDelete(nullptr);
}

}  // namespace

extern "C" void app_main(void) {
  c4001::UartConfig config;
  config.port = UART_NUM_1;
  config.tx_io_num = 2;
  config.rx_io_num = 3;
  config.baud_rate = 115200;

  static c4001::Service service;
  esp_err_t err = service.start(config, pdMS_TO_TICKS(500), 8);
  if (err != ESP_OK) {
    ESP_LOGE(kTag, "Failed to start c4001 service: %s", esp_err_to_name(err));
    return;
  }

  xTaskCreate(event_consumer, "c4001_logger", 4096, &service, 4, nullptr);
}
