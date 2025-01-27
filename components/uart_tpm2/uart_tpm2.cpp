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

// Initialisierung des statischen FIFOBuffer mit der Größe 16384 Bytes
FIFOBuffer UARTTPM2::fifo; // Definition der statischen Variable ohne Template-Parameter

void UARTTPM2::setup() {
  last_log_time_ = millis(); // Starte die Zeitmessung beim Setup
  resetReception();
}

void UARTTPM2::loop() 
{
    static uint32_t start_time = 0; // Zeit, wann wir angefangen haben, auf weitere Daten zu warten
    loop_start_time_ = millis(); // Zeitstempel für den Schleifenbeginn

    if (auto_mode_enabled_flag_ && last_package_processed_ > 0 && loop_start_time_ - last_package_processed_ >= 500)
    {
        ESP_LOGD("uart_tpm2", "Timeout, fordere TPM2 Paket an");
        get_one_tpm2_package(); // startet bzw. alle 1/2 sekunden ein Paket wenn nichts mehr kommt.
    }
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
            ESP_LOGI("uart_tpm2", "UART Lesezeit: %u msec | Buffer gelesen: %u", millis() - loop_start_time_, buffer_size);
        }
        // Debug: Logge die geschriebenen Bytes
        //ESP_LOGD("uart_tpm2", "Geschrieben: %u Bytes", written_bytes);

        if (written_bytes != buffer_size) {
            ESP_LOGW("uart_tpm2", "Nicht alle Bytes konnten in den FIFO-Puffer geschrieben werden. Gelesen: %u, Geschrieben: %u", 
                     buffer_size, written_bytes);
        }
    }

    puffer_size_start_ = fifo.getSize();
    if (puffer_size_start_ < 1200 && puffer_size_start_ > 0)
    {
      //ESP_LOGW("uart_tpm2", "Zu wenig gepuffert: %u Bytes | UART Puffer: %u", puffer_size_start_, available_bytes); 
      //return;
    }

    while (fifo.available()) {
        unsigned char c = fifo.read();
    
        if (receiving_) 
        {
            // ... (restlicher Code bleibt gleich)
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