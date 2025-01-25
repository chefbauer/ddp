#pragma once

#include "esphome.h"
#include <vector>

namespace esphome {
namespace uart_tpm2 {

class UARTTPM2 : public Component, public uart::UARTDevice {
 public:
  UARTTPM2() : UARTDevice(nullptr) {}
  void setup() override;
  void loop() override;
  void get_one_tpm2_package();
  static unsigned char it_bg[1350]; // Interne öffentliche und statische Variable

 private:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  static const int max_packet_size_ = 1350; // 450 * 3 bytes per color (RGB)
  unsigned char it_intern_[1350]; // Interne Variable als unsigned char Array
  uint32_t last_log_time_ = 0; // Zeitstempel für das letzte Log
  uint32_t frames_processed_ = 0; // Anzahl der verarbeiteten Frames
  uint32_t frames_dropped_ = 0; // Anzahl der verworfenen Frames

  void processTPM2Packet(const char* packet, int size);
  void resetReception();
  void log_frame_stats();
};

}  // namespace uart_tpm2
}  // namespace esphome