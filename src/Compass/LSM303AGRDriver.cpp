/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Driver for LSM303AGR                                          *
 * Author:   Ronan Demoment, Dietmar Warning                               *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021 by Ronan Demoment                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "LSM303AGRDriver.h"
#include "BoardConfig.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

// Magnetic register map I2C address
#define LSM303AGR_MAG_ADDR 0x1E
// Linear acceleration register map I2C addresses
#define LSM303AGR_ACC_ADDR 0x19

// LSM303AGR linear acceleration register map
#define STATUS_REG_AUX_A  0x07
#define OUT_TEMP_L_A      0x0c
#define OUT_TEMP_H_A      0x0d
#define INT_COUNTER_REG_A 0x0e
#define WHO_AM_I_A        0x0f
#define TEMP_CFG_REG_A    0x1f
#define CTRL_REG1_A       0x20
#define CTRL_REG2_A       0x21
#define CTRL_REG3_A       0x22
#define CTRL_REG4_A       0x23
#define CTRL_REG5_A       0x24
#define CTRL_REG6_A       0x25
#define REFERENCE_A       0x26
#define STATUS_REG_A      0x27
#define OUT_X_L_A         0x28
#define OUT_X_H_A         0x29
#define OUT_Y_L_A         0x2a
#define OUT_Y_H_A         0x2b
#define OUT_Z_L_A         0x2c
#define OUT_Z_H_A         0x2d
#define FIFO_CTRL_REG_A   0x2e
#define FIFO_SRC_REG_A    0x2f
#define INT1_CFG_A        0x30
#define INT1_SRC_A        0x31
#define INT1_THS_A        0x32
#define INT1_DURATION_A   0x33
#define INT2_CFG_A        0x34
#define INT2_SRC_A        0x35
#define INT2_THS_A        0x36
#define INT2_DURATION_A   0x37
#define CLICK_CFG_A       0x38
#define CLICK_SRC_A       0x39
#define CLICK_THS_A       0x3a
#define TIME_LIMIT_A      0x3b
#define TIME_LATENCY_A    0x3c
#define TIME_WINDOW_A     0x3d
#define ACT_THS_A         0x3e
#define ACT_DUR_A         0x3f

// LSM303AGR magnetic register map
#define OFFSET_X_REG_L_M 0x45
#define OFFSET_X_REG_H_M 0x46
#define OFFSET_Y_REG_L_M 0x47
#define OFFSET_Y_REG_H_M 0x48
#define OFFSET_Z_REG_L_M 0x49
#define OFFSET_Z_REG_H_M 0x4a
#define WHO_AM_I_M       0x4f
#define CFG_REG_A_M      0x60
#define CFG_REG_B_M      0x61
#define CFG_REG_C_M      0x62
#define INT_CRTL_REG_M   0x63
#define INT_SOURCE_REG_M 0x64
#define INT_THS_L_REG_M  0x65
#define INT_THS_H_REG_M  0x66
#define STATUS_REG_M     0x67
#define OUTX_L_REG_M     0x68
#define OUTX_H_REG_M     0x69
#define OUTY_L_REG_M     0x6a
#define OUTY_H_REG_M     0x6b
#define OUTZ_L_REG_M     0x6c
#define OUTZ_H_REG_M     0x6d

#define MAG_GAUSS_PER_LSB 0.0015f

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                           Static & Globals                              */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

// Constructor
LSM303AGRDriver::LSM303AGRDriver() : i2cMaster(nullptr), magDeviceHandle(nullptr), accDeviceHandle(nullptr), GPerLsb(1.0f)
{
}

// Destructor
LSM303AGRDriver::~LSM303AGRDriver()
{
    if (i2cMaster != nullptr)
    {
        if (accDeviceHandle != nullptr)
        {
            i2cMaster->UnregisterSlave(accDeviceHandle);
        }
        if (magDeviceHandle != nullptr)
        {
            i2cMaster->UnregisterSlave(magDeviceHandle);
        }
        i2cMaster = nullptr;
    }
}

// Initialization function
// Returns true if LSM303AGR has been found on the I2C bus, false else
bool LSM303AGRDriver::Init(I2cMaster *i2cMaster)
{
    uint8_t whoami = 0;

    this->i2cMaster = i2cMaster;

    accDeviceHandle = i2cMaster->RegisterSlave(LSM303AGR_ACC_ADDR, 100000);
    magDeviceHandle = i2cMaster->RegisterSlave(LSM303AGR_MAG_ADDR, 100000);
    // Check that we can read linear acceleration I2C address
    if (!i2cMaster->I2CRead(accDeviceHandle, WHO_AM_I_A, &whoami))
    {
        UnregisterSlaves();
        return false;
    }
    // Then check that we have the correct WHO_AM_I register value
    if (whoami != 0x33)
    {
        UnregisterSlaves();
        return false;
    }

    // Check that we can read magnetic I2C address
    if (!i2cMaster->I2CRead(magDeviceHandle, WHO_AM_I_M, &whoami))
    {
        UnregisterSlaves();
        return false;
    }
    // Then check that we have the correct WHO_AM_I register value
    if (whoami != 0x40)
    {
        UnregisterSlaves();
        return false;
    }

    // Check that we can read linear acceleration I2C address
    if (!i2cMaster->I2CRead(accDeviceHandle, WHO_AM_I_A, &whoami))
    {
        UnregisterSlaves();
        return false;
    }
    // Then check that we have the correct WHO_AM_I register value
    if (whoami != 0x33)
    {
        UnregisterSlaves();
        return false;
    }

    // Okay : if we are here, we have a LSM303AGR

    // Initialize registers and conversion coefficients
    i2cMaster->I2CWrite(magDeviceHandle, 0x00, CFG_REG_A_M);    // 0x00 ODR 10Hz, continuous mode
    i2cMaster->I2CWrite(magDeviceHandle, 0x01, CFG_REG_C_M);    // 0x10 BDU enabled
    i2cMaster->I2CWrite(magDeviceHandle, 0x00, INT_CRTL_REG_M); // 0x00 No interrupt handling

    i2cMaster->I2CWrite(accDeviceHandle, 0x27, CTRL_REG1_A); // 0x47 Normal Mode, ODR 50hz, all axes on
    i2cMaster->I2CWrite(accDeviceHandle, 0x00, CTRL_REG4_A); // 0x00 Range: +/-2 Gal, Sens.: 1mGal/LSB
    GPerLsb = 1.0 / 16384.0;

    return true;
}

// Return the name of the driven device
std::string LSM303AGRDriver::GetDeviceName()
{
    return (std::string("LSM303AGR"));
}

// Returns magnetic field measurements on X, Y and Z axis
// Unit is Gauss
void LSM303AGRDriver::GetMagneticField(Vec3D *mag)
{
    uint8_t magBuffer[6];
    int16_t mx, my, mz;

    // Single read of all magnetic measurements
    i2cMaster->I2CBurstRead(magDeviceHandle, OUTX_L_REG_M, magBuffer, 6);

    // Rebuilt integers values
    mx = ((int16_t)(magBuffer[1] << 8)) | (magBuffer[0] & 0xff);
    my = ((int16_t)(magBuffer[3] << 8)) | (magBuffer[2] & 0xff);
    mz = ((int16_t)(magBuffer[5] << 8)) | (magBuffer[4] & 0xff);

    // Convert to proper unit (Gauss)
    mag->x = mx * MAG_GAUSS_PER_LSB;
    mag->y = -my * MAG_GAUSS_PER_LSB;
    mag->z = mz * MAG_GAUSS_PER_LSB;
}

// Returns linear acceleration measurements on X, Y and Z axis
// Unit is G
void LSM303AGRDriver::GetAcceleration(Vec3D *acc)
{
    int16_t ax, ay, az;
    uint8_t regValue = 0;

    // Read of all acceleration measurements
    // X axis
    i2cMaster->I2CRead(accDeviceHandle, OUT_X_H_A, &regValue);
    ax = regValue;
    i2cMaster->I2CRead(accDeviceHandle, OUT_X_L_A, &regValue);
    ax = (ax << 8) | regValue;
    // Y axis
    i2cMaster->I2CRead(accDeviceHandle, OUT_Y_H_A, &regValue);
    ay = regValue;
    i2cMaster->I2CRead(accDeviceHandle, OUT_Y_L_A, &regValue);
    ay = (ay << 8) | regValue;
    // Z axis
    i2cMaster->I2CRead(accDeviceHandle, OUT_Z_H_A, &regValue);
    az = regValue;
    i2cMaster->I2CRead(accDeviceHandle, OUT_Z_L_A, &regValue);
    az = (az << 8) | regValue;

    // Convert to G
    acc->x = ax * GPerLsb;
    acc->y = -ay * GPerLsb;
    acc->z = az * GPerLsb;
}

void LSM303AGRDriver::UnregisterSlaves()
{
    if (i2cMaster != nullptr)
    {
        if (accDeviceHandle != nullptr)
        {
            i2cMaster->UnregisterSlave(accDeviceHandle);
            accDeviceHandle = nullptr;
        }
        if (magDeviceHandle != nullptr)
        {
            i2cMaster->UnregisterSlave(magDeviceHandle);
            magDeviceHandle = nullptr;
        }
    }
}
