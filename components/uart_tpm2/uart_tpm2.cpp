#include "uart_tpm2.h"

namespace esphome {
namespace uart_tpm2 {

// Hier ist die Definition der statischen Variable erforderlich
Color UARTTPM2::it_bg[450];

void UARTTPM2::setup() {
  // Starte den Stream, falls nötig
  resetReception();
}

void UARTTPM2::loop() 
{
    while (available()) 
    {
        char c = read();
        if (receiving_) 
        {
            current_packet_.push_back(c);
            if (current_packet_.size() >= 4) // Mindestens Header + Paketgröße
            {
                if (current_packet_[0] == 0xC9 && current_packet_[1] == 0xDA) 
                {
                    uint16_t data_size = (current_packet_[2] << 8) | current_packet_[3];
                    uint16_t expected_size = 2 + 2 + data_size + 1; // Header(2) + Paketgröße(2) + Daten(data_size) + Endbyte(1)
                    // if (current_packet_.size() == 4) // Nur bei der ersten Überprüfung nach Header
                    // {
                    //     ESP_LOGI("uart_tpm2", "Header und Paketgröße korrekt, Farbinformationen: %d", data_size);
                    // }
                    if (current_packet_.size() == expected_size) // Paket vollständig
                    {
                        if (c == 0x36) // Endbyte
                        {
                            receiving_ = false;
                            ESP_LOGI("uart_tpm2", "Korrekt empfangen: %d Farben", data_size);
                            processTPM2Packet(std::vector<char>(current_packet_.begin() + 4, current_packet_.end() - 1));
                            // Gibt die Kontrolle an den Scheduler zurück
                            yield();
                        }
                        resetReception(); // Paket verarbeitet oder ungültig
                        return; // Beende die Schleife, um ESPHome einen Durchlauf zu ermöglichen
                    }
                } 
                else 
                {
                    resetReception(); // Ungültiger Header, reset und warte auf neuen Start
                }
            }
        } 
        else if (c == 0xC9) // Startbyte 1
        {
            current_packet_.push_back(c);
            receiving_ = true;
        } 
        else if (current_packet_.size() == 1 && c == 0xDA) // Startbyte 2 nach 0xC9
        {
            current_packet_.push_back(c);
        } 
        else 
        {
            current_packet_.clear(); // Unerwartetes Zeichen vor Startbytes
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
  //ESP_LOGD("uart_tpm2", "Processed %d colors", data_index / 3);
}

void UARTTPM2::resetReception() {
  current_packet_.clear();
  receiving_ = false;
}

}  // namespace uart_tpm2
}  // namespace esphome