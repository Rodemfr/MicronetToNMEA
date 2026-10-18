#include "EspSerial.h"

#include <cstdarg>
#include <stdio.h>
#include <string.h>

EspSerial::EspSerial(uart_port_t uartNum, gpio_num_t rxPin, gpio_num_t txPin) : _uartNum(uartNum), _rxPin(rxPin), _txPin(txPin), _initialized(false)
{
}

void EspSerial::start(uint32_t baudRate)
{
    uart_config_t uartConfig = {.baud_rate           = static_cast<int>(baudRate),
                                .data_bits           = UART_DATA_8_BITS,
                                .parity              = UART_PARITY_DISABLE,
                                .stop_bits           = UART_STOP_BITS_1,
                                .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
                                .rx_flow_ctrl_thresh = 0,
                                .source_clk          = UART_SCLK_DEFAULT,
                                .flags               = {.allow_pd = 0, .backup_before_sleep = 0}};
    if (!_initialized)
    {
        uart_driver_install(_uartNum, 1024, 1024, 0, nullptr, 0);
        uart_param_config(_uartNum, &uartConfig);
        uart_set_pin(_uartNum, _txPin, _rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

        _initialized = true;
    }
    else
    {
        uart_param_config(_uartNum, &uartConfig);
    }

    _initialized = true;
}

void EspSerial::stop()
{
    if (!_initialized)
        return;

    uart_driver_delete(_uartNum);
    _initialized = false;
}

size_t EspSerial::printf(const char *format, ...)
{
    if (format == nullptr)
        return 0;

    char buffer[1024];

    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (length <= 0)
        return 0;

    return write(reinterpret_cast<const uint8_t *>(buffer), static_cast<size_t>(length));
}

int EspSerial::available()
{
    if (!_initialized)
        return 0;

    size_t length = 0;

    esp_err_t err = uart_get_buffered_data_len(_uartNum, &length);

    if (err != ESP_OK)
        return 0;

    return static_cast<int>(length);
}

size_t EspSerial::write(uint8_t value)
{
    std::lock_guard<std::mutex> lock(_writeMutex);

    if (!_initialized)
        return 0;

    esp_err_t err = uart_write_bytes(_uartNum, &value, 1);

    return (err == ESP_OK) ? 1 : 0;
}

size_t EspSerial::write(const uint8_t *data, size_t len)
{
    std::lock_guard<std::mutex> lock(_writeMutex);

    if (!_initialized || data == nullptr || len == 0)
        return 0;

    esp_err_t err = uart_write_bytes(_uartNum, reinterpret_cast<const char *>(data), len);

    return (err == ESP_OK) ? len : 0;
}

int EspSerial::read()
{
    if (!_initialized)
        return -1;

    uint8_t data;

    int len = uart_read_bytes(_uartNum, &data, 1,
                              portMAX_DELAY); // Bloquant jusqu'à réception d'un octet

    if (len == 1)
        return static_cast<int>(data);

    return -1;
}
