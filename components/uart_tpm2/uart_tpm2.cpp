#pragma once

#include "esphome.h"
#include <vector>

using namespace esphome;

class UARTTPM2 : public Component {
 public:
  void setup() override;
  void loop() override;
  void start() { 
    Serial.write('O');  // Sende 'O' zum Starten des Streams
    stopped_ = false;    // Setze Flag, dass wir gestartet sind
  }
  void stop() { 
    Serial.write('o');   // Sende 'o' zum Stoppen des Streams
    stopped_ = true;     // Setze Flag, dass wir gestoppt sind
  }

 private:
  std::vector<char> current_packet_;
  bool receiving_ = false;
  bool stopped_ = false;  // Flag für den Stopp-Zustand
  static const int max_packet_size_ = 512 * 3; // 3 bytes per color (RGB)

  void processTPM2Packet(const std::vector<char>& packet);
  void resetReception();
};

void UARTTPM2::setup() {
  Serial.begin(1500000); // Initialisiere UART
  start();                // Starte den Stream beim Setup
  resetReception();
}

void UARTTPM2::loop() {
  if (!stopped_) {  // Nur wenn wir nicht gestoppt sind, verarbeiten wir Daten
    while (Serial.available() > 0) {
      char c = Serial.read();
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