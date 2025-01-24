#pragma once

#include "esphome.h"
#include <vector>

using namespace esphome;

class UARTTPM2 : public Component, public uart::UARTDevice {
 public:
  UARTTPM2(uart::UARTComponent *parent) : UARTDevice(parent) {}
  void setup() override;
  void loop() override;
  void start();
  void stop();

 private:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  bool stopped_ = false;  // Flag für den Stopp-Zustand
  static const int max_packet_size_ = 512 * 3; // 3 bytes per color (RGB)

  void processTPM2Packet(const std::vector<char>& packet);
  void resetReception();
};