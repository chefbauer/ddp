void UARTTPM2::loop() 
{
    static uint32_t start_time = 0; // Zeit, wann wir angefangen haben, auf weitere Daten zu warten

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
                    
                    if (current_packet_.size() >= expected_size) // Paket vollständig oder mehr Daten verfügbar
                    {
                        if (current_packet_.back() == 0x36) // Endbyte
                        {
                            receiving_ = false;
                            frames_processed_++;
                            processTPM2Packet(std::vector<char>(current_packet_.begin() + 4, current_packet_.end() - 1));
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
                                processTPM2Packet(std::vector<char>(current_packet_.begin() + 4, current_packet_.end()));
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
                        start_time = millis(); // Setze die Startzeit neu
                        return; // Beende die Schleife und warte auf weitere Daten
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
            else
            {
                // Wir haben weniger als 4 Bytes, wir warten weiterhin auf Daten
                start_time = millis(); // Setze die Startzeit neu
                return; // Beende die Schleife und warte auf weitere Daten
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

    // Logge die Statistik alle 5 Sekunden
    uint32_t now = millis();
    if (now - last_log_time_ >= 5000) {
        log_frame_stats();
        last_log_time_ = now;
        frames_processed_ = 0; // Zurücksetzen der Frames für die nächste Periode
        frames_dropped_ = 0; // Zurücksetzen der verworfenen Frames
    }
}