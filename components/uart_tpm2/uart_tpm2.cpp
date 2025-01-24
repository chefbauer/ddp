#include "uart_tpm2.h"

void UARTTPM2::setup() {
  // Starte den Stream, falls nötig
  resetReception();
}

void UARTTPM2::loop() {
  while (available()) {
    char c = read();
    if (receiving_) {
      if (current_packet_.size() < max_packet_size_) {
        if (c == 'E') { // Ende des Datenpakets
          receiving_ = false;
          processTPM2Packet(current_packet_);
        } else {
          current_packet_.push_back(c);
        }
      } else {
        // Pufferüberlauf
        ESP_LOGE("uart_tpm2", "Packet size exceeded max size, resetting");
        resetReception();
      }
    } else if (c == 'D') { // Start des Datenpakets
      resetReception();
      receiving_ = true;
    } else {
      // Unerwartetes Zeichen vor 'D'
      ESP_LOGW("uart_tpm2", "Unexpected character before start of packet: %c", c);
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