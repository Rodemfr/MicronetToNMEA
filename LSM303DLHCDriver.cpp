/*
 * LSM303DLHCDriver.cpp
 *
 *  Created on: 11 sept. 2021
 *      Author: Ronan
 */

#include "LSM303DLHCDriver.h"
#include "BoardConfig.h"
#include <Wire.h>

#define LSM303DLHC_MAG_ADDR   0x1E
#define LSM303DLHC_ACC_ADDR   0x19

#define LSM303DLHC_WHO_AM_I 0x3C

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
#define TEMP_OUT_H_M      0x31
#define TEMP_OUT_L_M      0x32

LSM303DLHCDriver::LSM303DLHCDriver() :
		accAddr(LSM303DLHC_ACC_ADDR), magAddr(LSM303DLHC_MAG_ADDR), magX(0), magY(0), magZ(0), accX(0), accY(0), accZ(0), LSB_per_Gauss_XY(1100.0f), LSB_per_Gauss_Z(980.0f), mGal_per_LSB(1.0f)
{
}

LSM303DLHCDriver::~LSM303DLHCDriver()
{
}

bool LSM303DLHCDriver::Init()
{
	uint8_t ira, irb, irc, sr, whoami;

	NAVCOMPASS_I2C.begin();

	if (!I2CRead(LSM303DLHC_ACC_ADDR, STATUS_REG_A, &sr))
	{
		return false;
	}
	accAddr = LSM303DLHC_ACC_ADDR;

	I2CRead(LSM303DLHC_MAG_ADDR, IRA_REG_M, &ira);
	I2CRead(LSM303DLHC_MAG_ADDR, IRB_REG_M, &irb);
	I2CRead(LSM303DLHC_MAG_ADDR, IRC_REG_M, &irc);

	if ((ira != 'H') || (irb != '4') || (irc != '3'))
	{
		return false;
	}

	magAddr = LSM303DLHC_MAG_ADDR;

	I2CRead(magAddr, WHO_AM_I_M, &whoami);

	if (whoami != LSM303DLHC_WHO_AM_I)
	{
		return false;
	}

	// DLHC Acceleration register
	I2CWrite(accAddr, 0x47, CTRL_REG1_A); // 0x47=0b01000111 Normal Mode, ODR 50Hz, all axes on
	I2CWrite(accAddr, 0x08, CTRL_REG4_A); // 0x08=0b00001000 Range: +/-2 Gal, Sens.: 1mGal/LSB, highRes on
	mGal_per_LSB = 1.0f;
	// DLHC Magnetic register
	I2CWrite(magAddr, 0x90, CRA_REG_M); // 0x90=0b10010000 ODR 15Hz, temperature sensor on
	I2CWrite(magAddr, 0x20, CRB_REG_M); // 0x20=0b00100000 Range: +/-1.3 Gauss gain: 1100LSB/Gauss
	LSB_per_Gauss_XY = 1100.0f;
	LSB_per_Gauss_Z = 980.0f;
	I2CWrite(magAddr, 0x00, MR_REG_M);    // Continuous mode

	return true;
}

string LSM303DLHCDriver::GetDeviceName()
{
	return (string("LSM303DLHC"));
}

void LSM303DLHCDriver::GetMagneticField(float *magX, float *magY, float *magZ)
{
	uint8_t magBuffer[6];
	int16_t mx, my, mz;

	I2CBurstRead(magAddr, OUT_X_H_M, magBuffer, 6);

	mx = ((int16_t) (magBuffer[0] << 8)) | magBuffer[1];
	mz = ((int16_t) (magBuffer[2] << 8)) | magBuffer[3]; // stupid change in order for DLHC
	my = ((int16_t) (magBuffer[4] << 8)) | magBuffer[5];

	*magX = (float) mx;
	*magY = (float) my;
	*magZ = (float) mz;
}

void LSM303DLHCDriver::GetAcceleration(float *accX, float *accY, float *accZ)
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

	*accX = (float) (ax >> 4); // DLHC registers contain a left-aligned 12-bit number, so values should be shifted right by 4 bits (divided by 16)
	*accY = (float) (ay >> 4);
	*accZ = (float) (az >> 4);
}

bool LSM303DLHCDriver::I2CRead(uint8_t i2cAddress, uint8_t address, uint8_t *data)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	if (NAVCOMPASS_I2C.endTransmission() != 0)
	{
		return false;
	}
	NAVCOMPASS_I2C.requestFrom(i2cAddress, (uint8_t) 1);
	*data = NAVCOMPASS_I2C.read();

	return (NAVCOMPASS_I2C.endTransmission() == 0);
}

bool LSM303DLHCDriver::I2CBurstRead(uint8_t i2cAddress, uint8_t address, uint8_t *buffer, uint8_t length)
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

bool LSM303DLHCDriver::I2CWrite(uint8_t i2cAddress, uint8_t data, uint8_t address)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	NAVCOMPASS_I2C.write(data);
	return (NAVCOMPASS_I2C.endTransmission() == 0);
}
