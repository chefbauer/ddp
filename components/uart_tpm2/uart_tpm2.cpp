#include "uart_tpm2.h"
#include "fifo_buffer.h"
#include <cstring> // Für memcpy
//#include <Arduino.h> // Für millis()

// #include "esp_timer.h"
// uint32_t millis() {
//     return (uint32_t)(esp_timer_get_time() / 1000ULL);
// }

namespace esphome {
namespace uart_tpm2 {


// Definition der statischen Variable
unsigned char UARTTPM2::it_bg[1350];

void UARTTPM2::setup() {
  last_log_time_ = millis(); // Starte die Zeitmessung beim Setup
  resetReception();
}

FIFOBuffer UARTTPM2::fifo(16384);  // Initialisierung der statischen Variable mit einer Größe von 16KB

void UARTTPM2::loop() 
{
    static uint32_t start_time = 0; // Zeit, wann wir angefangen haben, auf weitere Daten zu warten
    loop_start_time_ = millis(); // Zeitstempel für den Schleifenbeginn

    int available_bytes = available();
    if (available_bytes > 0) {
        // Puffergröße bestimmen und sicherstellen, dass wir nicht mehr lesen, als wir Puffer haben
        size_t buffer_size = std::min(static_cast<size_t>(available_bytes), 4096u); // Beispiel: bis zu 1024 Bytes auf einmal
        unsigned char buffer[buffer_size]; // Puffer für UART-Daten
        
        // Debug: Logge die gelesenen Bytes
        ESP_LOGD("uart_tpm2", "buffer_size: %u Bytes", buffer_size);

        // Lies alle verfügbaren Daten auf einmal in den Buffer
        size_t read_bytes = read_array(buffer, buffer_size);

        // Debug: Logge die gelesenen Bytes
        ESP_LOGD("uart_tpm2", "Gelesen: %u Bytes", read_bytes);

        size_t written_bytes = fifo.write(buffer, read_bytes);

        // Debug: Logge die geschriebenen Bytes
        ESP_LOGD("uart_tpm2", "Geschrieben: %u Bytes", written_bytes);

        if (written_bytes != read_bytes) {
            ESP_LOGW("uart_tpm2", "Nicht alle Bytes konnten in den FIFO-Puffer geschrieben werden. Gelesen: %u, Geschrieben: %u", 
                     read_bytes, written_bytes);
        }
    }

    puffer_size_start_ = fifo.getSize();
    if (puffer_size_start_ < 4000 && puffer_size_start_ > 0)
    {
      ESP_LOGW("uart_tpm2", "Zu wenig gepuffert: %u Bytes | UART Puffer: %u", puffer_size_start_, available_bytes); 
      //return;
    }


    // while (available()) 
    // {
    //     char c = read();
    // // Verarbeite Daten aus dem FIFO-Puffer, wenn verfügbar
    while (fifo.available()) {
        unsigned char c = fifo.read();
    
        if (receiving_) 
        {
            current_packet_.push_back(c);
            if (current_packet_.size() >= 4) // Mindestens Header + Paketgröße
            {
                if (current_packet_[0] == 0xC9 && current_packet_[1] == 0xDA) 
                {
                    uint16_t data_size = (current_packet_[2] << 8) | current_packet_[3];
                    uint16_t expected_size = 2 + 2 + data_size + 1; // Header(2) + Paketgröße(2) + Daten(data_size) + Endbyte(1)

                    if (current_packet_.size() >= expected_size) // Paket vollständig oder mehr Daten verfügbar
                    {
                        if (current_packet_.back() == 0x36) // Endbyte
                        {
                            receiving_ = false;
                            frames_processed_++;
                            processTPM2Packet(current_packet_.data() + 4, data_size); // Skip 4 bytes (Header + Size)
                            resetReception(); // Paket verarbeitet
                            return; // Beende die Schleife, um ESPHome eine Chance zu geben, andere Aufgaben zu verarbeiten
                        }
                        else if (current_packet_.size() > expected_size)
                        {
                            // Paket ist größer als erwartet, schneide das überschüssige Byte ab und behalte es für das nächste Paket
                            char next_byte = current_packet_.back();
                            current_packet_.pop_back();
                            if (current_packet_.back() == 0x36) 
                            {
                                receiving_ = false;
                                frames_processed_++;
                                processTPM2Packet(current_packet_.data() + 4, data_size);
                                resetReception(); // Paket verarbeitet
                                // Start a new packet with the extra byte
                                if (next_byte == 0xC9) 
                                {
                                    current_packet_.push_back(next_byte);
                                    receiving_ = true;
                                }
                                return; // Beende die Schleife
                            }
                            else
                            {
                                // Paket ist ungültig, resetten
                                ESP_LOGW("uart_tpm2", "Ungültiges Paket, zu viele Daten");
                                frames_dropped_++;
                                resetReception(); // Reset und warte auf neues Paket
                                if (next_byte == 0xC9) 
                                {
                                    current_packet_.push_back(next_byte);
                                    receiving_ = true;
                                }
                                return; // Beende die Schleife
                            }
                        }
                    }
                    else
                    {
                        // Paket ist noch nicht vollständig, warten wir
                    }
                } 
                else 
                {
                    // Ungültiger Header, aber wir behalten das erste Byte, um zu sehen, ob es der Beginn eines neuen Pakets ist
                    if (current_packet_[0] != 0xC9) 
                    {
                        ESP_LOGW("uart_tpm2", "Ungültiger Header");
                        frames_dropped_++;
                        resetReception(); // Reset und warte auf neues Paket
                    }
                    else if (current_packet_.size() == 1 && c != 0xDA) 
                    {
                        // Das erste Byte ist 0xC9, aber das zweite ist nicht 0xDA, resetten
                        ESP_LOGW("uart_tpm2", "Erwartet 0xDA nach 0xC9, aber bekommen %02X", (unsigned char)c);
                        frames_dropped_++;
                        resetReception(); // Reset und warte auf neues Paket
                    }
                    // Wenn hierher kommt, bedeutet es, dass wir 0xC9 erhalten haben und warten auf 0xDA
                }
            }
        } 
        else // Wir sind nicht im Empfangsmodus
        {
            if (c == 0xC9) // Startbyte 1
            {
                current_packet_.push_back(c);
                receiving_ = true;
                start_time = millis(); // Setze den Startzeitpunkt, wenn wir anfangen, ein Paket zu empfangen
            }
            // Andere Zeichen ignorieren, bis wir 0xC9 sehen
        }
    }

}

void UARTTPM2::processTPM2Packet(const char* packet, int size) 
{
    size_t bytes_to_copy = std::min(size, 1350);
    memcpy(it_bg, packet, bytes_to_copy); 
    //ESP_LOGD("uart_tpm2", "Processed %d colors", bytes_to_copy / 3);

    // Logge die Statistik alle 5 Sekunden
    uint32_t now = millis();
    if (now - last_log_time_ >= 5000) {
        log_frame_stats();
        last_log_time_ = now;
        frames_processed_ = 0; // Zurücksetzen der Frames für die nächste Periode
        frames_dropped_ = 0; // Zurücksetzen der verworfenen Frames
    }
}
void UARTTPM2::resetReception() 
{
    current_packet_.clear();
    receiving_ = false;
}

void UARTTPM2::log_frame_stats() {
    float fps = frames_processed_ / 5.0; // 5 Sekunden, daher teilen wir durch 5
    const size_t max_buffer_size = 16384; // 16KB = 16384 Bytes
    size_t buffer_used = available();
    float buffer_fill_percent = (static_cast<float>(buffer_used) / max_buffer_size) * 100.0;
    
    ESP_LOGI("uart_tpm2", "Frames pro Sekunde: %.2f, Verworfen: %d, Puffer %%: %.2f%% | Zeit ms: %d | Pkt. Start: %d", fps, frames_dropped_, buffer_fill_percent, millis() - loop_start_time_, puffer_size_start_);
}

void UARTTPM2::get_one_tpm2_package() {
    write(0x4C); // Sende das Zeichen 0x4C per UART
    //ESP_LOGI("uart_tpm2", "Gesendet: 0x4C");
}

}  // namespace uart_tpm2
}  // namespace esphome