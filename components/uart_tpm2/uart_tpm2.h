#pragma once

#include "esphome.h"
#include <vector>

using namespace esphome;

class UARTTPM2 : public Component {
 public:
  void setup() override;
  void loop() override;

 private:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  static const int max_packet_size_ = 512 * 3; // 3 bytes per color (RGB)

  void processTPM2Packet(const std::vector<char>& packet);
  void resetReception();
};