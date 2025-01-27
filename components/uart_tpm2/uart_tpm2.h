#pragma once

#include "esphome.h"
#include <cstddef>
#include "fifo_buffer.h" // Sicherstellen, dass Sie die richtige Header-Datei einbinden, die den Template enthält

namespace esphome {
namespace uart_tpm2 {

class UARTTPM2 : public Component, public uart::UARTDevice {
 public:
  UARTTPM2() : UARTDevice(nullptr) {}
  void setup() override;
  void loop() override;
  void get_one_tpm2_package();
  void auto_mode_enable(int auto_mode_fps_target = 0);
  void auto_mode_disable();
  static unsigned char it_bg[1350]; // Interne öffentliche und statische Variable

 private:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  bool auto_mode_enabled_flag_ = false;
  int auto_mode_fps_target_ = 0;
  static const int max_packet_size_ = 1350; // 450 * 3 bytes per color (RGB)
  unsigned char it_intern_[1350]; // Interne Variable als unsigned char Array
  uint32_t last_log_time_ = 0; // Zeitstempel für das letzte Log
  uint32_t last_package_processed_ = 0; // Zeitstempel für das letzte Log
  uint32_t loop_start_time_ = 0; // Zeitstempel Start
  int puffer_size_start_ = 0; // Wieviele Bytes im Puffer
  uint32_t frames_processed_ = 0; // Anzahl der verarbeiteten Frames
  uint32_t frames_dropped_ = 0; // Anzahl der verworfenen Frames
  
  // FIFO PUFFER
  // Deklaration der statischen Variable mit festgelegter Größe, z.B. 4096 Bytes
  static DRAM_ATTR FIFOBuffer<16384> fifo;

  void processTPM2Packet(const unsigned char* packet, int size); // Geändert zu unsigned char*
  void resetReception();
  void log_frame_stats();
};

}  // namespace uart_tpm2
}  // namespace esphome