#include "uart_tpm2.h"

namespace esphome {
namespace uart_tpm2 {

void UARTTPM2::setup() {
  // Starte den Stream, falls nötig
  resetReception();
}

void UARTTPM2::loop() {
  while (available()) {
    char c = read();
    if (receiving_) {
      current_packet_.push_back(c);
      if (current_packet_.size() >= 4) { // Mindestens Header + Paketgröße
        if (current_packet_[0] == 0xC9 && current_packet_[1] == 0xDA) 
        {
          ESP_LOGW("uart_tpm2", "Starte TPM2 Empfang");

          uint16_t packet_size = (current_packet_[2] << 8) | current_packet_[3];
          if (current_packet_.size() == packet_size * 3 + 5) { // Gesamtpaketgröße (Header + Daten + Endbyte) 5 ned 6, daten sind extra!
            if (c == 0x36) { // Endbyte
              receiving_ = false;
              processTPM2Packet(std::vector<char>(current_packet_.begin() + 4, current_packet_.end() - 1));
            }
          }
        }
        else {
          // Paket ist ungültig, reset und warte auf neuen Start
          ESP_LOGW("uart_tpm2", "Paket ist ungültig, reset und warte auf neuen Start");
          resetReception();
        }
      }
    } else if (c == 0xC9) { // Startbyte 1
      current_packet_.clear();
      current_packet_.push_back(c);
      receiving_ = true;
    } else if (current_packet_.size() == 1 && c == 0xDA) { // Startbyte 2 nach 0xC9
      current_packet_.push_back(c);
    } else {
      // Unerwartetes Zeichen vor Startbytes
      ESP_LOGW("uart_tpm2", "Unexpected character before start of packet: %c", c);
      current_packet_.clear();
    }
  }
}

void UARTTPM2::processTPM2Packet(const std::vector<char>& packet) {
  int data_index = 0;
  for (int i = 0; i < std::min((int)packet.size() / 3, 450); ++i) {
    if (data_index + 2 < packet.size()) {
      // Speichere die empfangenen Farbdaten in die interne Variable
      it_bg[i][0] = packet[data_index];
      it_bg[i][1] = packet[data_index + 1];
      it_bg[i][2] = packet[data_index + 2];
      data_index += 3;
    }
  }
  // Logge die Anzahl der verarbeiteten Farben
  ESP_LOGD("uart_tpm2", "Processed %d colors", data_index / 3);
}

void UARTTPM2::resetReception() {
  current_packet_.clear();
  receiving_ = false;
}

}  // namespace uart_tpm2
}  // namespace esphome