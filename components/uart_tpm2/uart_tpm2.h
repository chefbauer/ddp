#pragma once

#include "esphome.h"
#include <vector>

using namespace esphome;

class UARTTPM2 : public Component, public uart::UARTDevice {
 public:
  UARTTPM2(uart::UARTComponent *parent) : UARTDevice(parent) {}
  void setup() override;
  void loop() override;
  unsigned char it_bg[450][3]; // Interne öffentliche Variable

 private:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  static const int max_packet_size_ = 450 * 3; // 3 bytes per color (RGB)

  void processTPM2Packet(const std::vector<char>& packet);
  void resetReception();
};