#pragma once

#include "esphome.h"
#include "esphome/core/color.h" // include für die Color-Klasse
#include <vector>

namespace esphome {
namespace uart_tpm2 {

class UARTTPM2 : public Component, public uart::UARTDevice {
 public:
  UARTTPM2() : UARTDevice(nullptr) {}
  void setup() override;
  void loop() override;
  void get_one_tpm2_package();
  static Color it_bg[450]; // Interne öffentliche Variable

 private:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  static const int max_packet_size_ = 450 * 3; // 3 bytes per color (RGB)
  Color it_intern_[450]; // Interne Variable
  uint32_t last_log_time_ = 0; // Zeitstempel für das letzte Log
  uint32_t frames_processed_ = 0; // Anzahl der verarbeiteten Frames
  uint32_t frames_dropped_ = 0; // Anzahl der verworfenen Frames

  void processTPM2Packet(const std::vector<char>& packet);
  void resetReception();
  void log_frame_stats();
};

}  // namespace uart_tpm2
}  // namespace esphome