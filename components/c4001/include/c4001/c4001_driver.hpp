#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <cstddef>
#include <cstdint>

#include "c4001/c4001_protocol.hpp"
#include "driver/uart.h"
#include "esp_err.h"

namespace c4001 {

struct UartConfig {
  uart_port_t port = UART_NUM_1;
  int tx_io_num = 17;
  int rx_io_num = 16;
  int baud_rate = 115200;
  uart_word_length_t data_bits = UART_DATA_8_BITS;
  uart_parity_t parity = UART_PARITY_DISABLE;
  uart_stop_bits_t stop_bits = UART_STOP_BITS_1;
  uart_hw_flowcontrol_t flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  size_t rx_buffer_size = 256;
  size_t tx_buffer_size = 256;
};

struct PresenceSample {
  TickType_t timestamp;
  PresenceReport report;
};

struct HeartbeatSample {
  TickType_t timestamp;
  HeartbeatReport report;
};

class Driver {
 public:
  Driver();
  ~Driver();

  esp_err_t init(const UartConfig &config);
  esp_err_t sendCommand(Command cmd, const uint8_t *payload, size_t payload_len);
  esp_err_t readFrame(Frame *frame, TickType_t timeout);
  esp_err_t queryPresence(PresenceReport *out, TickType_t timeout);
  esp_err_t queryHeartbeat(HeartbeatReport *out, TickType_t timeout);

 private:
  esp_err_t readBytes(uint8_t *buffer, size_t len, TickType_t timeout);

  uart_port_t port_;
  bool initialized_;
};

}  // namespace c4001
