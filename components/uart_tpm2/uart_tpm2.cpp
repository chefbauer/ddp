#include "uart_tpm2.h"
#include "fifo_buffer.h"
#include <cstring> // Für memcpy
#include <cstddef>

#include "esp_timer.h"
#ifndef ARDUINO
uint32_t millis() {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}
#endif

namespace esphome {
namespace uart_tpm2 {

// Definition der statischen Variable
unsigned char UARTTPM2::it_bg[1350];
int color_size_target = 396*3; //1188
int package_size_target = color_size_target+4+2;

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
    if (auto_mode_fps_target_ == 0)
    {
      auto_mode_fps_target_ = 1;
    }
    int fps_wait_time_msec = 1000 / auto_mode_fps_target_;
    int time_diff = millis() - last_package_processed_time_;
    // Fordere alle fps_wait_time_msec msec Pakete an bis der Puffer über 3* Paketgröße ist.
    if (auto_mode_enabled_flag_ && (puffer_size_start_ < (3*package_size_target)) && (time_diff > fps_wait_time_msec))
    {
      //ESP_LOGW("uart_tpm2", "Zu wenig gepuffert: %u Bytes | UART Puffer: %u", puffer_size_start_, available_bytes); 
      //ESP_LOGW("uart_tpm2", "Fordere neues Paket an (Wartezeit: %u) | FPS Target: %u | time_diff: %u | Puffer: %u", fps_wait_time_msec, auto_mode_fps_target_, time_diff, puffer_size_start_); 
      get_one_tpm2_package(); // nur ein Ping :)
    }    

    //Abbruch wenn zu wenig im Puffer ODER fps zeit pass ned!
    if ((puffer_size_start_ < package_size_target) || (auto_mode_enabled_flag_ && time_diff < fps_wait_time_msec))
    {
      return;
    }
    else if (puffer_size_start_ < package_size_target)
    {
      return;
    }
    // Non-destruktiver Scan: readAt() prüft ohne Bytes zu konsumieren.
    // Erst wenn Header UND Endbyte validiert sind, werden Bytes verbraucht.
    // Dadurch kein "False-Sync" auf 0xC9 innerhalb von RGB-Nutzdaten und
    // kein Index-OOB wenn das Paket noch nicht vollstaendig im FIFO ist.
    while (fifo.available())
    {
        // Mindestens 4 Bytes fuer den Header notwendig
        if (fifo.getSize() < 4) return;

        // TPM2-Header prüfen (0xC9 0xDA)
        if (fifo.readAt(0) != 0xC9 || fifo.readAt(1) != 0xDA) {
            // Kein gueltiger Header-Start – ein Byte ueberspringen und von vorne
            fifo.advanceReadPos(1);
            frames_dropped_++;
            continue;
        }

        // Paketgroesse lesen (kein Sign-Extension-Problem, da readAt() unsigned char zurueckgibt)
        uint16_t data_size = ((uint16_t)fifo.readAt(2) << 8) | (uint16_t)fifo.readAt(3);

        // Plausibilitaet: nur die erwartete Groesse akzeptieren
        if (data_size != (uint16_t)color_size_target) {
            ESP_LOGD("uart_tpm2", "Ungueltiger data_size %u (erwartet %d), ueberspringe 1 Byte", data_size, color_size_target);
            fifo.advanceReadPos(1);
            frames_dropped_++;
            continue;
        }

        // Warten bis das komplette Paket da ist: Header(4) + Nutzdaten(data_size) + Endbyte(1)
        if (fifo.getSize() < (size_t)(4 + data_size + 1)) {
            return; // Unvollstaendiges Paket – beim naechsten loop()-Aufruf weitermachen
        }

        // Endbyte 0x36 an der erwarteten Position prüfen
        if (fifo.readAt(4 + data_size) != 0x36) {
            ESP_LOGD("uart_tpm2", "Falsches Endbyte 0x%02X an Pos %u, ueberspringe 1 Byte", fifo.readAt(4 + data_size), 4 + data_size);
            fifo.advanceReadPos(1);
            frames_dropped_++;
            continue;
        }

        // Gueltiges Paket – jetzt erst Bytes konsumieren
        fifo.advanceReadPos(4);      // Header (0xC9 0xDA size_hi size_lo) ueberspringen
        fifo.read(it_bg, data_size); // Nutzdaten direkt in it_bg lesen
        fifo.advanceReadPos(1);      // Endbyte 0x36 ueberspringen
        frames_processed_++;

        // Statistik alle 5 Sekunden loggen
        uint32_t now = millis();
        if (now - last_log_time_ >= 5000) {
            log_frame_stats();
            last_log_time_ = now;
            frames_processed_ = 0;
            frames_dropped_ = 0;
        }
        last_package_processed_time_ = millis();
        resetReception();
        return; // ESPHome andere Aufgaben erledigen lassen
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
    
    ESP_LOGI("uart_tpm2", "Frames pro Sekunde: %.2f, Verworfen: %d, Puffer %%: %.2f%% | Zeit ms: %ld | Pkt. Start: %d", fps, frames_dropped_, buffer_fill_percent, millis() - loop_start_time_, puffer_size_start_);
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

void UARTTPM2::setTargetFPS(int auto_mode_fps_target)
{
  auto_mode_fps_target_ = auto_mode_fps_target; // noch nicht implementiert
}

void UARTTPM2::sendJson(const char* jsonText) {
  if (jsonText != nullptr) {
    size_t length = strlen(jsonText);
    for (size_t i = 0; i < length; ++i) {
      write(jsonText[i]);  // Jeden Char einzeln schreiben
    }
    //write('\n');  // Optional: Neue Zeile oder Trennzeichen hinzufügen
  }
}

}  // namespace uart_tpm2
}  // namespace esphome