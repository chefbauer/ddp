#include "uart_tpm2.h"
#include <cstring> // Für memcpy
#include <Arduino.h> // Für millis()

namespace esphome {
namespace uart_tpm2 {

// Definition der statischen Variable
Color UARTTPM2::it_bg[450];

void UARTTPM2::setup() {
  last_log_time_ = millis(); // Starte die Zeitmessung beim Setup
  resetReception();
}

void UARTTPM2::loop() 
{
    static uint32_t start_time = 0; // Zeit, wann wir angefangen haben, auf weitere Daten zu warten

    if (receiving_) 
    {
        if (current_packet_.size() >= 4) // Mindestens Header + Paketgröße
        {
            if (current_packet_[0] == 0xC9 && current_packet_[1] == 0xDA) 
            {
                uint16_t data_size = (current_packet_[2] << 8) | current_packet_[3];
                uint16_t expected_size = 2 + 2 + data_size + 1; // Header(2) + Paketgröße(2) + Daten(data_size) + Endbyte(1)
                
                while (current_packet_.size() < expected_size && (millis() - start_time < 4)) // Warten bis zu 4ms
                {
                    if (available()) 
                    {
                        char c = read();
                        current_packet_.push_back(c);
                        start_time = millis(); // Reset der Zeit, wenn neue Daten kommen
                    }
                }

                if (current_packet_.size() >= expected_size) // Paket vollständig oder mehr Daten verfügbar
                {
                    if (current_packet_.back() == 0x36) // Endbyte
                    {
                        receiving_ = false;
                        frames_processed_++;
                        processTPM2Packet(std::vector<char>(current_packet_.begin() + 4, current_packet_.end() - 1));
                        resetReception(); // Paket verarbeitet
                        return;
                    }
                }
                else
                {
                    // Timeout erreicht, Paket ist nicht vollständig
                    ESP_LOGW("uart_tpm2", "Paket unvollständig, Timeout erreicht");
                    frames_dropped_++;
                    resetReception(); // Reset und warte auf neues Paket
                    return;
                }
            }
            else 
            {
                // Ungültiger Header, aber wir behalten die Daten, um zu sehen, ob es der Beginn eines neuen Pakets ist
                ESP_LOGW("uart_tpm2", "Ungültiger Header");
                frames_dropped_++;
                resetReception(); // Reset und warte auf neues Paket
            }
        }

        // Wenn wir hierher kommen, haben wir nicht genug Daten im Puffer, also lesen wir weiter
        while (available()) 
        {
            char c = read();
            current_packet_.push_back(c);
        }
    } 
    else // Wir sind nicht im Empfangsmodus
    {
        while (available()) 
        {
            char c = read();
            if (c == 0xC9) // Startbyte 1
            {
                current_packet_.push_back(c);
                receiving_ = true;
                start_time = millis(); // Setze den Startzeitpunkt, wenn wir anfangen, ein Paket zu empfangen
            } 
            else 
            {
                // Ignoriere alle anderen Zeichen, bis wir 0xC9 sehen
            }
        }
    }

    // Logge die Statistik alle 5 Sekunden
    uint32_t now = millis();
    if (now - last_log_time_ >= 5000) {
        log_frame_stats();
        last_log_time_ = now;
        frames_processed_ = 0; // Zurücksetzen der Frames für die nächste Periode
        frames_dropped_ = 0; // Zurücksetzen der verworfenen Frames
    }
}

void UARTTPM2::processTPM2Packet(const std::vector<char>& packet) 
{
    int data_index = 0;
    for (int i = 0; i < std::min((int)packet.size() / 3, 450); ++i) {
        if (data_index + 2 < packet.size()) {
            it_intern_[i].r = packet[data_index];
            it_intern_[i].g = packet[data_index + 1];
            it_intern_[i].b = packet[data_index + 2];
            data_index += 3;
        }
    }
    memcpy(it_bg, it_intern_, sizeof(Color) * 450); // memcpy wird verwendet, da es in der Regel schneller ist
    ESP_LOGD("uart_tpm2", "Processed %d colors", data_index / 3);
}

void UARTTPM2::resetReception() 
{
    current_packet_.clear();
    receiving_ = false;
}

void UARTTPM2::log_frame_stats() {
    float fps = frames_processed_ / 5.0; // 5 Sekunden, daher teilen wir durch 5
    ESP_LOGI("uart_tpm2", "Frames pro Sekunde: %.2f, Verworfen: %d", fps, frames_dropped_);
}

// Nicht-statische Methode, um 0x4C zu senden
void UARTTPM2::get_one_tpm2_package() {
    write(0x4C); // Sende das Zeichen 0x4C per UART
    //ESP_LOGI("uart_tpm2", "Gesendet: 0x4C");
}

}  // namespace uart_tpm2
}  // namespace esphome