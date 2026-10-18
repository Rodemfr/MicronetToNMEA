/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Driver for LSM303DLH                                          *
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

#include "LSM303DLHDriver.h"
#include "BoardConfig.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

// Magnetic register map I2C address
#define LSM303DLH_MAG_ADDR 0x1E
// Linear acceleration register map I2C addresses
#define LSM303DLH_ACC_ADDR   0x18
#define LSM303DLH_ACC_ADDR_1 0x19

// Value returned by LSM303DLHC when querying WHO_AM_I register
#define LSM303DLHC_WHO_AM_I 0x3C

// LSM303DLH linear acceleration register map
#define CTRL_REG1_A       0x20
#define CTRL_REG2_A       0x21
#define CTRL_REG3_A       0x22
#define CTRL_REG4_A       0x23
#define CTRL_REG5_A       0x24
#define HP_FILTER_RESET_A 0x25
#define REFERENCE_A       0x26
#define STATUS_REG_A      0x27
#define OUT_X_L_A         0x28
#define OUT_X_H_A         0x29
#define OUT_Y_L_A         0x2a
#define OUT_Y_H_A         0x2b
#define OUT_Z_L_A         0x2c
#define OUT_Z_H_A         0x2d
#define INT1_CFG_A        0x30
#define INT1_SOURCE_A     0x31
#define INT1_THS_A        0x32
#define INT1_DURATION_A   0x33
#define INT2_CFG_A        0x34
#define INT2_SOURCE_A     0x35
#define INT2_THS_A        0x36
#define INT2_DURATION_A   0x37

// LSM303DLH magnetic register map
#define CRA_REG_M  0x00
#define CRB_REG_M  0x01
#define MR_REG_M   0x02
#define OUT_X_H_M  0x03
#define OUT_X_L_M  0x04
#define OUT_Y_H_M  0x05
#define OUT_Y_L_M  0x06
#define OUT_Z_H_M  0x07
#define OUT_Z_L_M  0x08
#define SR_REG_M   0x09
#define IRA_REG_M  0x0a
#define IRB_REG_M  0x0b
#define IRC_REG_M  0x0c
#define WHO_AM_I_M 0x0f

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
LSM303DLHDriver::LSM303DLHDriver() : i2cMaster(nullptr), LsbPerGaussXY(1100.0f), LsbPerGaussZ(980.0f), GPerLsb(1.0f)
{
}

// Destructor
LSM303DLHDriver::~LSM303DLHDriver()
{
}

// Initialization function
// Returns true if LSM303DLH has been found on the I2C bus, false else
bool LSM303DLHDriver::Init(I2cMaster *i2cMaster)
{
    uint8_t ira = 0, irb = 0, irc = 0, sr, whoami = 0;

    this->i2cMaster = i2cMaster;

    accDeviceHandle = i2cMaster->RegisterSlave(LSM303DLH_ACC_ADDR_1, 100000);
    magDeviceHandle = i2cMaster->RegisterSlave(LSM303DLH_MAG_ADDR, 100000);
    // First we search for linear acceleration address which depends on SA0 pin level on LSM303DLH
    if (!i2cMaster->I2CRead(accDeviceHandle, STATUS_REG_A, &sr))
    {
        i2cMaster->UnregisterSlave(accDeviceHandle);
        accDeviceHandle = i2cMaster->RegisterSlave(LSM303DLH_ACC_ADDR, 100000);
        if (!i2cMaster->I2CRead(accDeviceHandle, STATUS_REG_A, &sr))
        {
            // Linear acceleration not found : return false
            UnregisterSlaves();
            return false;
        }
    }

    // Check that magnetic register have the correct identification ID
    i2cMaster->I2CRead(magDeviceHandle, IRA_REG_M, &ira);
    i2cMaster->I2CRead(magDeviceHandle, IRB_REG_M, &irb);
    i2cMaster->I2CRead(magDeviceHandle, IRC_REG_M, &irc);

    if ((ira != 'H') || (irb != '4') || (irc != '3'))
    {
        // No : return false
        UnregisterSlaves();
        return false;
    }

    // Check if we have foudn an LSM303DLH or LSM303DLHC
    i2cMaster->I2CRead(magDeviceHandle, WHO_AM_I_M, &whoami);

    if (whoami == LSM303DLHC_WHO_AM_I)
    {
        // This is a LSM303DLHC : return false
        i2cMaster->UnregisterSlave(accDeviceHandle);
        i2cMaster->UnregisterSlave(magDeviceHandle);
        return false;
    }

    // Okay : if we are here, we have a LSM303DLH

    // Initialize registers and conversion coefficients
    i2cMaster->I2CWrite(magDeviceHandle, 0x18, CRA_REG_M);   // 0x18=0b00011000 ODR 75Hz
    i2cMaster->I2CWrite(accDeviceHandle, 0x27, CTRL_REG1_A); // 0x27=0b00100111 Normal Mode, ODR 50hz, all axes on
    i2cMaster->I2CWrite(accDeviceHandle, 0x00, CTRL_REG4_A); // 0x00=0b00000000 Range: +/-2 Gal, Sens.: 1mGal/LSB
    GPerLsb = 1.0 / 16384.0;
    i2cMaster->I2CWrite(magDeviceHandle, 0x60, CRB_REG_M); // 0x60=0b01100000 Range: +/-2.5 Gauss gain: 635LSB/Gauss
    LsbPerGaussXY = 635;
    LsbPerGaussZ  = 570;
    i2cMaster->I2CWrite(magDeviceHandle, 0x00, MR_REG_M); // Continuous mode

    return true;
}

// Return the name of the driven device
std::string LSM303DLHDriver::GetDeviceName()
{
    return (std::string("LSM303DLH"));
}

// Returns magnetic field measurements on X, Y and Z axis
// Unit is Gauss
void LSM303DLHDriver::GetMagneticField(Vec3D *mag)
{
    uint8_t magBuffer[6];
    int16_t mx, my, mz;

    // Single read of all magnetic measurements
    i2cMaster->I2CBurstRead(magDeviceHandle, OUT_X_H_M, magBuffer, 6);

    // Rebuilt integers values
    mx = ((int16_t)(magBuffer[0] << 8)) | magBuffer[1];
    my = ((int16_t)(magBuffer[2] << 8)) | magBuffer[3];
    mz = ((int16_t)(magBuffer[4] << 8)) | magBuffer[5];

    // Convert to proper unit (Gauss)
    mag->x = -mx / LsbPerGaussXY;
    mag->y = -my / LsbPerGaussXY;
    mag->z = mz / LsbPerGaussZ;
}

// Returns linear acceleration measurements on X, Y and Z axis
// Unit is G
void LSM303DLHDriver::GetAcceleration(Vec3D *acc)
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
    acc->x = -ax * GPerLsb;
    acc->y = -ay * GPerLsb;
    acc->z = az * GPerLsb;
}

void LSM303DLHDriver::UnregisterSlaves()
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
