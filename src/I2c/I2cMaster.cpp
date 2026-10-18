#include "I2cMaster.h"
#include "BoardConfig.h"

extern "C"
{
#include "driver/i2c_master.h"
}

I2cMaster::I2cMaster() : busHandle(nullptr)
{
}

I2cMaster::~I2cMaster()
{
    DeInit();
}

bool I2cMaster::Init(i2c_port_t i2cPort, gpio_num_t sdaPin, gpio_num_t sclPin)
{
    if (busHandle != nullptr)
    {
        return true;
    }

    i2c_master_bus_config_t busConfig = {
        .i2c_port          = i2cPort,
        .sda_io_num        = sdaPin,
        .scl_io_num        = sclPin,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority     = 0,
        .trans_queue_depth = 0,
        .flags             = {
            .enable_internal_pullup = 1,
            .allow_pd               = 0,
        },
    };

    if (i2c_new_master_bus(&busConfig, &busHandle) != ESP_OK)
    {
        busHandle = nullptr;
    }

    return (busHandle != nullptr);
}

void I2cMaster::DeInit()
{
    if (busHandle != nullptr)
    {
        i2c_del_master_bus(busHandle);
        busHandle = nullptr;
    }
}

i2c_master_dev_handle_t I2cMaster::RegisterSlave(uint8_t i2cAddress, uint32_t speedHz)
{
    if (busHandle == nullptr)
    {
        return nullptr;
    }

    i2c_device_config_t deviceConfig = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = i2cAddress,
        .scl_speed_hz    = speedHz,
        .scl_wait_us     = 0,
        .flags          = {
            .disable_ack_check = 0,
        },
    };

    i2c_master_dev_handle_t deviceHandle;

    if (i2c_master_bus_add_device(busHandle, &deviceConfig, &deviceHandle) != ESP_OK)
    {
        return nullptr;
    }

    return deviceHandle;
}

void I2cMaster::UnregisterSlave(i2c_master_dev_handle_t deviceHandle)
{
    if (deviceHandle != nullptr)
    {
        i2c_master_bus_rm_device(deviceHandle);
    }
}

bool I2cMaster::I2CRead(I2cMasterDeviceHandle deviceHandle, uint8_t address, uint8_t *data)
{
    if ((deviceHandle == nullptr) || (data == nullptr))
    {
        return false;
    }

    uint8_t regAddress = address;
    if (i2c_master_transmit(deviceHandle, &regAddress, 1, 1000) != ESP_OK)
    {
        return false;
    }

    return (i2c_master_receive(deviceHandle, data, 1, 1000) == ESP_OK);
}

bool I2cMaster::I2CBurstRead(I2cMasterDeviceHandle deviceHandle, uint8_t address, uint8_t *buffer, uint8_t length)
{
    if ((deviceHandle == nullptr) || (buffer == nullptr) || (length == 0))
    {
        return false;
    }

    uint8_t regAddress = address;
    if (i2c_master_transmit(deviceHandle, &regAddress, 1, 1000) != ESP_OK)
    {
        return false;
    }

    return (i2c_master_receive(deviceHandle, buffer, length, 1000) == ESP_OK);
}

bool I2cMaster::I2CWrite(I2cMasterDeviceHandle deviceHandle, uint8_t data, uint8_t address)
{
    if (deviceHandle == nullptr)
    {
        return false;
    }

    uint8_t writeBuffer[2] = {address, data};
    return (i2c_master_transmit(deviceHandle, writeBuffer, sizeof(writeBuffer), 1000) == ESP_OK);
}
