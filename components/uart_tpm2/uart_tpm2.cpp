#include "uart_tpm2.h"

namespace esphome {
namespace uart_tpm2 {

void UARTTPM2::setup() {
  // Annahme: Der UART ist in der YAML-Konfiguration bereits initialisiert
  start();                // Starte den Stream beim Setup
  resetReception();
}

void UARTTPM2::loop() {
  if (!stopped_) {  // Nur wenn wir nicht gestoppt sind, verarbeiten wir Daten
    while (available() > 0) {
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
}

void UARTTPM2::start() { 
  write('O');  // Sende 'O' zum Starten des Streams
  stopped_ = false;    // Setze Flag, dass wir gestartet sind
}

void UARTTPM2::stop() { 
  write('o');   // Sende 'o' zum Stoppen des Streams
  stopped_ = true;     // Setze Flag, dass wir gestoppt sind
}

void UARTTPM2::processTPM2Packet(const std::vector<char>& packet) {
  int data_index = 0;
  for (int i = 0; i < std::min((int)packet.size() / 3, 512); ++i) {
    if (data_index + 2 < packet.size()) {
      // Annahme: Jede Farbe wird durch 3 aufeinanderfolgende Bytes (RGB) repräsentiert
      id(it_bg)[i].r = packet[data_index];
      id(it_bg)[i].g = packet[data_index + 1];
      id(it_bg)[i].b = packet[data_index + 2];
      data_index += 3;
    }
  }

  // Beispiel: Ausgabe der Anzahl der empfangenen Farben für Debugging
  ESP_LOGD("uart_tpm2", "Processed %d colors", data_index / 3);
}

void UARTTPM2::resetReception() {
  current_packet_.clear();
  receiving_ = false;
}

}  // namespace uart_tpm2
}  // namespace esphome