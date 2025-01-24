#pragma once

#include "esphome.h"
#include "esphome/core/color.h"
#include <vector>

namespace esphome {
namespace uart_tpm2 {

class UARTTPM2 : public Component, public uart::UARTDevice {
 public:
  UARTTPM2() : UARTDevice(nullptr) {}
  void setup() override;
  void loop() override;
//  static unsigned char it_bg[450][3]; // Interne öffentliche Variable
  static Color it_bg[450]; // Interne öffentliche Variable

 private:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  static const int max_packet_size_ = 450 * 3; // 3 bytes per color (RGB)

  void processTPM2Packet(const std::vector<char>& packet);
  void resetReception();
};

}  // namespace uart_tpm2
}  // namespace esphome