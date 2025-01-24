#include "uart_tpm2.h"

namespace esphome {
namespace uart_tpm2 {

// Definition der statischen Variable
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
                    //ESP_LOGI("uart_tpm2", "Header und Paketgröße korrekt, Farbinformationen: %d", data_size);
                    if (current_packet_.size() == expected_size) // Paket vollständig
                    {
                        if (c == 0x36) // Endbyte
                        {
                            receiving_ = false;
                            //ESP_LOGI("uart_tpm2", "Korrekt empfangen: %d Farben", data_size);
                            processTPM2Packet(std::vector<char>(current_packet_.begin() + 4, current_packet_.end() - 1));
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

void UARTTPM2::processTPM2Packet(const std::vector<char>& packet) 
{
    int data_index = 0;
    for (int i = 0; i < std::min((int)packet.size() / 3, 450); ++i) {
        if (data_index + 2 < packet.size()) {
            // Speichere die empfangenen Farbdaten in die interne Variable
            it_intern_[i].r = packet[data_index];
            it_intern_[i].g = packet[data_index + 1];
            it_intern_[i].b = packet[data_index + 2];
            data_index += 3;
        }
    }
    // Kopiere die Daten von der internen Variable in die öffentliche Variable
    memcpy(it_bg, it_intern_, sizeof(Color) * 450); // memcpy wird verwendet, da es in der Regel schneller ist
    // Logge die Anzahl der verarbeiteten Farben
    ESP_LOGD("uart_tpm2", "Processed %d colors", data_index / 3);
}

void UARTTPM2::resetReception() 
{
    current_packet_.clear();
    receiving_ = false;
}

// Neue public Funktion
void UARTTPM2::get_one_tpm2_package() {
    write(0x4C); // Sende das Zeichen 0x4C per UART
    ESP_LOGI("uart_tpm2", "Gesendet: 0x4C");
}

}  // namespace uart_tpm2
}  // namespace esphome