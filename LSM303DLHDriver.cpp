/*
 * LSM303DLHDriver.cpp
 *
 *  Created on: 11 sept. 2021
 *      Author: Ronan
 */

#include "LSM303DLHDriver.h"
#include "BoardConfig.h"
#include <Wire.h>

#define LSM303DLH_MAG_ADDR   0x1E
#define LSM303DLH_ACC_ADDR   0x18
#define LSM303DLH_ACC_ADDR_1 0x19

#define LSM303DLHC_WHO_AM_I 0x3C

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
#define CRA_REG_M         0x00
#define CRB_REG_M         0x01
#define MR_REG_M          0x02
#define OUT_X_H_M         0x03
#define OUT_X_L_M         0x04
#define OUT_Y_H_M         0x05
#define OUT_Y_L_M         0x06
#define OUT_Z_H_M         0x07
#define OUT_Z_L_M         0x08
#define SR_REG_M          0x09
#define IRA_REG_M         0x0a
#define IRB_REG_M         0x0b
#define IRC_REG_M         0x0c
#define WHO_AM_I_M        0x0f

LSM303DLHDriver::LSM303DLHDriver() :
		accAddr(LSM303DLH_ACC_ADDR), magAddr(LSM303DLH_MAG_ADDR), magX(0), magY(0), magZ(0), accX(0), accY(0), accZ(0), LSB_per_Gauss_XY(1100.0f), LSB_per_Gauss_Z(980.0f), mGal_per_LSB(1.0f)
{
}

LSM303DLHDriver::~LSM303DLHDriver()
{
}

bool LSM303DLHDriver::Init()
{
	uint8_t ira, irb, irc, sr, whoami;

	NAVCOMPASS_I2C.begin();

	if (!I2CRead(LSM303DLH_ACC_ADDR_1, STATUS_REG_A, &sr))
	{
		if (!I2CRead(LSM303DLH_ACC_ADDR, STATUS_REG_A, &sr))
		{
			return false;
		}
		accAddr = LSM303DLH_ACC_ADDR;
	}
	else
	{
		accAddr = LSM303DLH_ACC_ADDR_1;
	}

	I2CRead(LSM303DLH_MAG_ADDR, IRA_REG_M, &ira);
	I2CRead(LSM303DLH_MAG_ADDR, IRB_REG_M, &irb);
	I2CRead(LSM303DLH_MAG_ADDR, IRC_REG_M, &irc);

	if ((ira != 'H') || (irb != '4') || (irc != '3'))
	{
		return false;
	}

	magAddr = LSM303DLH_MAG_ADDR;

	I2CRead(magAddr, WHO_AM_I_M, &whoami);

	if (whoami == LSM303DLHC_WHO_AM_I)
	{
		return false;
	}

	// Magnetic register
	I2CWrite(magAddr, 0x18, CRA_REG_M);   // 0x18=0b00011000 ODR 75Hz
	I2CWrite(accAddr, 0x27, CTRL_REG1_A); // 0x27=0b00100111 Normal Mode, ODR 50hz, all axes on
	I2CWrite(accAddr, 0x00, CTRL_REG4_A); // 0x00=0b00000000 Range: +/-2 Gal, Sens.: 1mGal/LSB
	mGal_per_LSB = 1.0 / 16384.0;
	I2CWrite(magAddr, 0x60, CRB_REG_M);   // 0x60=0b01100000 Range: +/-2.5 Gauss gain: 635LSB/Gauss
	LSB_per_Gauss_XY = 635;
	LSB_per_Gauss_Z = 570;
	I2CWrite(magAddr, 0x00, MR_REG_M);    // Continuous mode

	return true;
}

string LSM303DLHDriver::GetDeviceName()
{
	return (string("LSM303DLH"));
}

void LSM303DLHDriver::GetMagneticField(float *magX, float *magY, float *magZ)
{
	uint8_t magBuffer[6];
	int16_t mx, my, mz;

	I2CBurstRead(magAddr, OUT_X_H_M, magBuffer, 6);

	mx = ((int16_t) (magBuffer[0] << 8)) | magBuffer[1];
	my = ((int16_t) (magBuffer[2] << 8)) | magBuffer[3];
	mz = ((int16_t) (magBuffer[4] << 8)) | magBuffer[5];

	*magX = -mx / LSB_per_Gauss_XY;
	*magY = -my / LSB_per_Gauss_XY;
	*magZ = mz / LSB_per_Gauss_Z;
}

void LSM303DLHDriver::GetAcceleration(float *accX, float *accY, float *accZ)
{
	int16_t ax, ay, az;
	uint8_t regValue;

	I2CRead(accAddr, OUT_X_H_A, &regValue);
	ax = regValue;
	I2CRead(accAddr, OUT_X_L_A, &regValue);
	ax = (ax << 8) | regValue;

	I2CRead(accAddr, OUT_Y_H_A, &regValue);
	ay = regValue;
	I2CRead(accAddr, OUT_Y_L_A, &regValue);
	ay = (ay << 8) | regValue;

	I2CRead(accAddr, OUT_Z_H_A, &regValue);
	az = regValue;
	I2CRead(accAddr, OUT_Z_L_A, &regValue);
	az = (az << 8) | regValue;

	*accX = ax * mGal_per_LSB;
	*accY = ay * mGal_per_LSB;
	*accZ = -az * mGal_per_LSB;
}

bool LSM303DLHDriver::I2CRead(uint8_t i2cAddress, uint8_t address, uint8_t *data)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	if (NAVCOMPASS_I2C.endTransmission() != 0)
	{
		return false;
	}
	NAVCOMPASS_I2C.requestFrom(i2cAddress, (uint8_t) 1);
	*data = NAVCOMPASS_I2C.read();
	NAVCOMPASS_I2C.endTransmission();

	return (NAVCOMPASS_I2C.endTransmission() == 0);
}

bool LSM303DLHDriver::I2CBurstRead(uint8_t i2cAddress, uint8_t address, uint8_t *buffer, uint8_t length)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	if (NAVCOMPASS_I2C.endTransmission() != 0)
	{
		return false;
	}
	NAVCOMPASS_I2C.requestFrom(i2cAddress, (uint8_t) length);
	NAVCOMPASS_I2C.readBytes(buffer, NAVCOMPASS_I2C.available());
	return (NAVCOMPASS_I2C.endTransmission() == 0);
}

bool LSM303DLHDriver::I2CWrite(uint8_t i2cAddress, uint8_t data, uint8_t address)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	NAVCOMPASS_I2C.write(data);
	return (NAVCOMPASS_I2C.endTransmission() == 0);
}
