#pragma once

#include "esphome.h"

namespace esphome {
namespace uart_tpm2 {

class UartTpm2 : public Component, public uart::UARTDevice {
public:
    void setup() override;
    void loop() override;
    void set_it_bg(unsigned char (*it_bg)[3]) { this->it_bg = it_bg; }

protected:
    unsigned char (*it_bg)[3] = nullptr;
};

} // namespace uart_tpm2
} // namespace esphome