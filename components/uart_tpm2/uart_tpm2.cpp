#include "uart_tpm2.h"
#include "fifo_buffer.h"
#include <cstring> // Für memcpy
#include <cstddef>

#include "esp_timer.h"
uint32_t millis() {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

namespace esphome {
namespace uart_tpm2 {

// Definition der statischen Variable
unsigned char UARTTPM2::it_bg[1350];
int package_size_target = 394*3+4+1;

// Initialisierung des statischen FIFOBuffer
FIFOBuffer UARTTPM2::fifo;

void UARTTPM2::setup() {
  last_log_time_ = millis(); // Starte die Zeitmessung beim Setup
  resetReception();
}

void UARTTPM2::loop() 
{
    static uint32_t start_time = 0; // Zeit, wann wir angefangen haben, auf weitere Daten zu warten
    loop_start_time_ = millis(); // Zeitstempel für den Schleifenbeginn

    // if (auto_mode_enabled_flag_ && last_package_processed_time_ > 0 && loop_start_time_ - last_package_processed_time_ >= 500)
    // {
    //     get_one_tpm2_package(); // startet bzw. alle 1/2 sekunden ein Paket wenn nichts mehr kommt.
    // }
                            
                            // if (auto_mode_enabled_flag_ && fps_wait_time_msec > (millis() - last_package_processed_time_))
                            // {
                            //       int puffer_size = fifo.getSize();
                            //       if (puffer_size < 2000)
                            //       {
                            //         ESP_LOGW("uart_tpm2", "Fordere neues Paket an (Wartezeit: %u) | FPS Target: %u", fps_wait_time_msec, auto_mode_fps_target_); 
                            //         get_one_tpm2_package(); // nur ein Ping :)
                            //       }
                            //   // reguliert sich so selbst!
                            // }

    int available_bytes = available();
    if (available_bytes > 0) {
        // Puffergröße bestimmen und sicherstellen, dass wir nicht mehr lesen, als wir Puffer haben
        size_t buffer_size = std::min(static_cast<size_t>(available_bytes), 8192u); // Beispiel: bis zu 8192 Bytes auf einmal
        unsigned char buffer[buffer_size]; // Puffer für UART-Daten
        
        // Debug: Logge die gelesenen Bytes
        //ESP_LOGD("uart_tpm2", "buffer_size: %u Bytes", buffer_size);

        // Lies alle verfügbaren Daten auf einmal in den Buffer
        bool read_success = read_array(buffer, buffer_size);
        size_t written_bytes = 0; // Initialisiere written_bytes
        if (read_success)
        {
            written_bytes = fifo.write(buffer, buffer_size);
            //ESP_LOGI("uart_tpm2", "UART Lesezeit: %u msec | Buffer gelesen: %u", millis() - loop_start_time_, buffer_size);
        }
        // Debug: Logge die geschriebenen Bytes
        //ESP_LOGD("uart_tpm2", "Geschrieben: %u Bytes", written_bytes);

        if (written_bytes != buffer_size) {
            ESP_LOGW("uart_tpm2", "Nicht alle Bytes konnten in den FIFO-Puffer geschrieben werden. Gelesen: %u, Geschrieben: %u", 
                     buffer_size, written_bytes);
        }
    }

    puffer_size_start_ = fifo.getSize();
    int fps_wait_time_msec = 1000 / auto_mode_fps_target_;
    int time_diff = millis() - last_package_processed_time_;
    // Fordere alle fps_wait_time_msec msec Pakete an bis der Puffer über 3* Paketgröße ist.
    if (auto_mode_enabled_flag_ && puffer_size_start_ < 3*package_size_target && time_diff > fps_wait_time_msec)
    {
      //ESP_LOGW("uart_tpm2", "Zu wenig gepuffert: %u Bytes | UART Puffer: %u", puffer_size_start_, available_bytes); 
      ESP_LOGW("uart_tpm2", "Fordere neues Paket an (Wartezeit: %u) | FPS Target: %u | time_diff: %u", fps_wait_time_msec, auto_mode_fps_target_, time_diff); 
      get_one_tpm2_package(); // nur ein Ping :)
    }    

    //Abbruch wenn zu wenig im Puffer!
    if (puffer_size_start_ < package_size_target)
    {
      return;
    }

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
                            memcpy(it_bg, current_packet_.data() + 4, data_size); // Direkte Kopie der Daten in it_bg
                            resetReception(); // Paket verarbeitet

                            // Logge die Statistik alle 5 Sekunden
                            uint32_t now = millis();
                            if (now - last_log_time_ >= 5000) {
                                log_frame_stats();
                                last_log_time_ = now;
                                frames_processed_ = 0; // Zurücksetzen der Frames für die nächste Periode
                                frames_dropped_ = 0; // Zurücksetzen der verworfenen Frames
                            }
                            last_package_processed_time_ = millis();
                            resetReception(); // Paket verarbeitet
                            return; // Beende die Schleife, um ESPHome eine Chance zu geben, andere Aufgaben zu verarbeiten
                        }
                        else
                        {
                            last_package_processed_time_ = millis();
                            resetReception(); // Paket verarbeitet
                            return; // Beende die Schleife, um ESPHome eine Chance zu geben, andere Aufgaben zu verarbeiten
                        }
                        // else if (current_packet_.size() > expected_size)
                        // {
                        //     // Paket ist größer als erwartet, schneide das überschüssige Byte ab und behalte es für das nächste Paket
                        //     char next_byte = current_packet_.back();
                        //     current_packet_.pop_back();
                        //     if (current_packet_.back() == 0x36) 
                        //     {
                        //         receiving_ = false;
                        //         frames_processed_++;
                        //         memcpy(it_bg, current_packet_.data() + 4, data_size); // Direkte Kopie der Daten in it_bg
                        //         resetReception(); // Paket verarbeitet
                        //         // Start a new packet with the extra byte
                        //         if (next_byte == 0xC9) 
                        //         {
                        //             current_packet_.push_back(next_byte);
                        //             receiving_ = true;
                        //         }
                        //         return; // Beende die Schleife
                        //     }
                        //     else
                        //     {
                        //         // Paket ist ungültig, resetten
                        //         ESP_LOGW("uart_tpm2", "Ungültiges Paket, zu viele Daten");
                        //         frames_dropped_++;
                        //         resetReception(); // Reset und warte auf neues Paket
                        //         return; // Beende die Schleife
                        //     }
                        // }
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

void UARTTPM2::processTPM2Packet(const unsigned char* packet, int size) 
{
    memcpy(it_bg, packet, size); // Direkte Kopie der Daten in it_bg
    ESP_LOGD("uart_tpm2", "Processed %d colors", size / 3);

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

void UARTTPM2::get_one_tpm2_package() 
{
    write(0x4C); // Sende das Zeichen 0x4C per UART
    //ESP_LOGI("uart_tpm2", "Gesendet: 0x4C");
}

void UARTTPM2::auto_mode_enable(int auto_mode_fps_target)
{
  auto_mode_fps_target_ = auto_mode_fps_target; // noch nicht implementiert
  auto_mode_enabled_flag_ = true;
}

void UARTTPM2::auto_mode_disable() {
  auto_mode_fps_target_ = 0;
  auto_mode_enabled_flag_ = false;
}

}  // namespace uart_tpm2
}  // namespace esphome