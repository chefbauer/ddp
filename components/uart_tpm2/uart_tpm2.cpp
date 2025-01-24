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
                    uint16_t expected_size = 2 + 2 + data_size * 3 + 1; // Header(2) + Paketgröße(2) + Daten(data_size*3) + Endbyte(1)
                    if (current_packet_.size() == 4) // Nur bei der ersten Überprüfung nach Header
                    {
                        ESP_LOGI("uart_tpm2", "Header und Paketgröße korrekt, Paketgröße: %d", data_size);
                    }
                    if (current_packet_.size() == expected_size) // Paket vollständig
                    {
                        if (c == 0x36) // Endbyte
                        {
                            receiving_ = false;
                            ESP_LOGI("uart_tpm2", "Korrekt empfangen: %d RGB-Werte", data_size);
                            processTPM2Packet(std::vector<char>(current_packet_.begin() + 4, current_packet_.end() - 1));
                        }
                        resetReception(); // Paket verarbeitet oder ungültig
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