#pragma once

#include "esphome.h"
#include <vector>

namespace esphome {
namespace uart_tpm2 {

class UARTTPM2 : public Component, public uart::UARTDevice {
 public:
  UARTTPM2(uart::UARTComponent *parent, Color* bg) : UARTDevice(parent), it_bg_(bg) {}
  void setup() override;
  void loop() override;
  void start();
  void stop();

 private:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  bool stopped_ = false;  // Flag für den Stopp-Zustand
  static const int max_packet_size_ = 512 * 3; // 3 bytes per color (RGB)
  Color* it_bg_;  // Pointer auf die globale Variable

  void processTPM2Packet(const std::vector<char>& packet);
  void resetReception();
};

}  // namespace uart_tpm2
}  // namespace esphome