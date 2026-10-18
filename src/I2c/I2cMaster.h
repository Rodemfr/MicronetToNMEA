
#pragma once

extern "C"
{
#include "driver/i2c_master.h"
}

typedef i2c_master_dev_handle_t I2cMasterDeviceHandle;

class I2cMaster
{
  public:
    I2cMaster();
    virtual ~I2cMaster();

    bool                  Init(i2c_port_t i2cPort, gpio_num_t sdaPin, gpio_num_t sclPin);
    void                  DeInit();
    I2cMasterDeviceHandle RegisterSlave(uint8_t i2cAddress, uint32_t speedHz);
    void                  UnregisterSlave(I2cMasterDeviceHandle deviceHandle);
    bool                  I2CRead(I2cMasterDeviceHandle deviceHandle, uint8_t address, uint8_t *data);
    bool                  I2CBurstRead(I2cMasterDeviceHandle deviceHandle, uint8_t address, uint8_t *buffer, uint8_t length);
    bool                  I2CWrite(I2cMasterDeviceHandle deviceHandle, uint8_t data, uint8_t address);

  private:
    i2c_master_bus_handle_t busHandle;
};
