#include "c4001/c4001_driver.hpp"

#include <algorithm>

#include "esp_check.h"
#include "esp_log.h"

namespace c4001 {
namespace {
constexpr const char *kTag = "c4001_driver";
constexpr TickType_t kDefaultUartTimeout = pdMS_TO_TICKS(100);
}

Driver::Driver() : port_(UART_NUM_1), initialized_(false) {}

Driver::~Driver() {
  if (initialized_) {
    uart_driver_delete(port_);
  }
}

esp_err_t Driver::init(const UartConfig &config) {
  port_ = config.port;

  uart_config_t uart_config = {
      .baud_rate = config.baud_rate,
      .data_bits = config.data_bits,
      .parity = config.parity,
      .stop_bits = config.stop_bits,
      .flow_ctrl = config.flow_ctrl,
      .rx_flow_ctrl_thresh = 0,
      .source_clk = UART_SCLK_DEFAULT,
      .flags = 0,
  };

  ESP_RETURN_ON_ERROR(uart_param_config(port_, &uart_config), kTag, "param config failed");
  ESP_RETURN_ON_ERROR(uart_set_pin(port_, config.tx_io_num, config.rx_io_num, UART_PIN_NO_CHANGE,
                                   UART_PIN_NO_CHANGE),
                      kTag, "set pin failed");
  ESP_RETURN_ON_ERROR(uart_driver_install(port_, config.rx_buffer_size, config.tx_buffer_size, 0, nullptr, 0),
                      kTag, "driver install failed");

  initialized_ = true;
  return ESP_OK;
}

esp_err_t Driver::sendCommand(Command cmd, const uint8_t *payload, size_t payload_len) {
  if (!initialized_) {
    return ESP_ERR_INVALID_STATE;
  }
  if (payload_len > kMaxPayloadSize) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t buffer[2 + 1 + 1 + kMaxPayloadSize + 1 + 2];
  size_t offset = 0;
  buffer[offset++] = kFrameHeader0;
  buffer[offset++] = kFrameHeader1;
  buffer[offset++] = static_cast<uint8_t>(payload_len + 1 /*cmd*/ + 1 /*checksum*/ + 2 /*tail*/);
  buffer[offset++] = static_cast<uint8_t>(cmd);
  if (payload_len) {
    std::copy(payload, payload + payload_len, buffer + offset);
  }
  offset += payload_len;
  buffer[offset] = calc_checksum(buffer + 2, payload_len + 2);
  offset += 1;
  buffer[offset++] = kFrameTail0;
  buffer[offset++] = kFrameTail1;

  int written = uart_write_bytes(port_, buffer, offset);
  if (written < 0 || static_cast<size_t>(written) != offset) {
    ESP_LOGE(kTag, "uart_write_bytes failed: %d", written);
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t Driver::readFrame(Frame *frame, TickType_t timeout) {
  if (!initialized_ || !frame) {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t header[2];
  ESP_RETURN_ON_ERROR(readBytes(header, sizeof(header), timeout), kTag, "header timeout");
  if (header[0] != kFrameHeader0 || header[1] != kFrameHeader1) {
    ESP_LOGW(kTag, "invalid header %02x %02x", header[0], header[1]);
    return ESP_ERR_INVALID_RESPONSE;
  }

  uint8_t length = 0;
  ESP_RETURN_ON_ERROR(readBytes(&length, 1, timeout), kTag, "length timeout");

  uint8_t command = 0;
  ESP_RETURN_ON_ERROR(readBytes(&command, 1, timeout), kTag, "command timeout");
  frame->command = static_cast<Command>(command);
  frame->length = length;

  size_t payload_len = std::min(static_cast<size_t>(length - 3), kMaxPayloadSize);
  if (payload_len > 0) {
    ESP_RETURN_ON_ERROR(readBytes(frame->payload.data(), payload_len, timeout), kTag, "payload timeout");
  }

  ESP_RETURN_ON_ERROR(readBytes(&frame->checksum, 1, timeout), kTag, "checksum timeout");

  uint8_t tail[2];
  ESP_RETURN_ON_ERROR(readBytes(tail, sizeof(tail), timeout), kTag, "tail timeout");
  if (tail[0] != kFrameTail0 || tail[1] != kFrameTail1) {
    ESP_LOGW(kTag, "invalid tail %02x %02x", tail[0], tail[1]);
    return ESP_ERR_INVALID_RESPONSE;
  }

  uint8_t computed = calc_checksum(&length, 1);
  computed += static_cast<uint8_t>(frame->command);
  for (size_t i = 0; i < payload_len; ++i) {
    computed += frame->payload[i];
  }
  if (computed != frame->checksum) {
    ESP_LOGW(kTag, "checksum mismatch %02x != %02x", computed, frame->checksum);
    return ESP_ERR_INVALID_CRC;
  }
  return ESP_OK;
}

esp_err_t Driver::queryPresence(PresenceReport *out, TickType_t timeout) {
  uint8_t dummy = 0;
  ESP_RETURN_ON_ERROR(sendCommand(Command::kQueryPresence, &dummy, 0), kTag, "presence command failed");
  Frame frame;
  ESP_RETURN_ON_ERROR(readFrame(&frame, timeout), kTag, "presence frame failed");
  if (frame.command != Command::kQueryPresence) {
    return ESP_ERR_INVALID_RESPONSE;
  }
  out->state = static_cast<PresenceState>(frame.payload[0]);
  out->distance_cm = (frame.payload[1] << 8) | frame.payload[2];
  return ESP_OK;
}

esp_err_t Driver::queryHeartbeat(HeartbeatReport *out, TickType_t timeout) {
  uint8_t dummy = 0;
  ESP_RETURN_ON_ERROR(sendCommand(Command::kQueryHeartbeat, &dummy, 0), kTag, "heartbeat command failed");
  Frame frame;
  ESP_RETURN_ON_ERROR(readFrame(&frame, timeout), kTag, "heartbeat frame failed");
  if (frame.command != Command::kQueryHeartbeat) {
    return ESP_ERR_INVALID_RESPONSE;
  }
  out->human_present = frame.payload[0];
  out->bpm = (frame.payload[1] << 8) | frame.payload[2];
  out->confidence = frame.payload[3];
  return ESP_OK;
}

esp_err_t Driver::readBytes(uint8_t *buffer, size_t len, TickType_t timeout) {
  size_t received = 0;
  const TickType_t deadline = xTaskGetTickCount() + (timeout ? timeout : kDefaultUartTimeout);
  while (received < len) {
    TickType_t now = xTaskGetTickCount();
    if (timeout && now >= deadline) {
      return ESP_ERR_TIMEOUT;
    }
    int chunk = uart_read_bytes(port_, buffer + received, len - received, pdMS_TO_TICKS(20));
    if (chunk < 0) {
      return ESP_FAIL;
    }
    received += chunk;
  }
  return ESP_OK;
}

}  // namespace c4001
