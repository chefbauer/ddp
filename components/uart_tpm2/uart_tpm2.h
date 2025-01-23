#pragma once

// #include <array>
// #include <cstdint>
// #include <map>
// #ifndef COMPONENT_OBIS_D0_OPTIMIZE_SIZE
//     #include <regex>
// #endif
// #include <string>

// #include "esphome/components/uart/uart.h"
// #include "esphome/core/automation.h"
// #include "esphome/core/component.h"

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace esphome {
namespace uart_tpm2 {

class UARTTPM2 : public uart::UARTDevice {
 public:
  UARTTPM2(uart::UARTComponent *parent) : UARTDevice(parent) {}

  void setup() override;
  void loop() override;

 protected:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  static const int max_packet_size_ = 512 * 3; // 3 bytes per color (RGB)

  void processTPM2Packet(const std::vector<char>& packet);
  void resetReception();
};

}  // namespace uart_tpm2
}  // namespace esphome