#pragma once

extern "C"
{
#include "driver/gpio.h"
#include "driver/uart.h"
}

#include <cstddef>
#include <mutex>
#include <stdint.h>
#include <string>

class EspSerial
{
  public:
    EspSerial(uart_port_t uartNum, gpio_num_t rxPin, gpio_num_t txPin);

    void start(uint32_t baudRate);
    void stop();

    size_t printf(const char *format, ...);

    size_t write(uint8_t byte);
    size_t write(const uint8_t *data, size_t len);
    int    read();
    int    available();

  private:
    uart_port_t _uartNum;
    gpio_num_t  _rxPin;
    gpio_num_t  _txPin;
    bool        _initialized;
    std::mutex  _writeMutex;
};