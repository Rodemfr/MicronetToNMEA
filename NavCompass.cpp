/*
 * CompassDecoder.cpp
 *
 *  Created on: 13 juin 2021
 *      Author: Ronan
 */

#include "NavCompass.h"
#include "BoardConfig.h"
#include "Globals.h"

#include <Wire.h>

#define LSM303DLH_MAG_ADDR  0x1E
#define LSM303DLH_ACC_ADDR  0x18

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

NavCompass::NavCompass() :
		heading(0), magX(0), magY(0), magZ(0), accX(0), accY(0), accZ(0)
{
}

NavCompass::~NavCompass()
{
}

bool NavCompass::Init()
{
	NAVCOMPASS_I2C.begin();

	uint8_t ira = I2CRead(LSM303DLH_MAG_ADDR, IRA_REG_M);
	uint8_t irb = I2CRead(LSM303DLH_MAG_ADDR, IRB_REG_M);
	uint8_t irc = I2CRead(LSM303DLH_MAG_ADDR, IRC_REG_M);

	if ((ira != 'H') || (irb != '4') || (irc != '3'))
	{
		return false;
	}

	I2CWrite(LSM303DLH_ACC_ADDR, 0x27, CTRL_REG1_A);  // 0x47 = ODR 50hz all axes on
	I2CWrite(LSM303DLH_MAG_ADDR, 0x10, CRA_REG_M);    // 15Hz
	I2CWrite(LSM303DLH_MAG_ADDR, 0x03, CRB_REG_M);    // Gauss range
	I2CWrite(LSM303DLH_MAG_ADDR, 0x00, MR_REG_M);     // Continuous mode

	return true;
}

void NavCompass::GetMagneticField(float *magX, float *magY, float *magZ)
{
	uint8_t magBuffer[6];
	int16_t mx, my, mz;

	I2CBurstRead(LSM303DLH_MAG_ADDR, OUT_X_H_M, magBuffer, 6);

	mx = ((int16_t) (magBuffer[0] << 8)) | magBuffer[1];
	my = ((int16_t) (magBuffer[2] << 8)) | magBuffer[3];
	mz = ((int16_t) (magBuffer[4] << 8)) | magBuffer[5];

	*magX = ((float) mx / 635.0f);
	*magY = ((float) my / 635.0f);
	*magZ = ((float) mz / 635.0f);
}

void NavCompass::GetAcceleration(float *accX, float *accY, float *accZ)
{
	int16_t ax, ay, az;

	ax = I2CRead(LSM303DLH_ACC_ADDR, OUT_X_H_A);
	ax = (ax << 8) | I2CRead(LSM303DLH_ACC_ADDR, OUT_X_L_A);
	ay = I2CRead(LSM303DLH_ACC_ADDR, OUT_Y_H_A);
	ay = (ay << 8) | I2CRead(LSM303DLH_ACC_ADDR, OUT_Y_L_A);
	az = I2CRead(LSM303DLH_ACC_ADDR, OUT_Z_H_A);
	az = (az << 8) | I2CRead(LSM303DLH_ACC_ADDR, OUT_Z_L_A);

	*accX = (float) -ax / 16384.0f;
	*accY = (float) -ay / 16384.0f;
	*accZ = (float) -az / 16384.0f;
}

float NavCompass::GetHeading()
{
	float magX, magY, magZ;
	float accelX, accelY, accelZ;
	float pBow, pStarboard;
	float starboardY, starboardZ;
	float starboardNorm;

	// Get Acceleration and Magnetic data from LSM303
	// Note that we don't care about units of both acceleration and magnetic field since we
	// are only calculating angles.
	GetAcceleration(&accelX, &accelY, &accelZ);
	GetMagneticField(&magX, &magY, &magZ);

	// Substract calibration offsets from magnetic readings
	magX -= gConfiguration.xMagOffset;
	magY -= gConfiguration.yMagOffset;
	magZ -= gConfiguration.zMagOffset;

	// TODO : filter data

	// Build starboard axis from boat's bow & gravity vector
	starboardY = accelZ;
	starboardZ = -accelY;
	starboardNorm = sqrtf(starboardY * starboardY + starboardZ * starboardZ);

	// Project magnetic field on bow & starboard axis
	pBow = magX;
	pStarboard = (magY * starboardY + magZ * starboardZ) / starboardNorm;

	float angle = atan2(-pStarboard, pBow) * 180 / M_PI;
	if (angle < 0)
		angle += 360;

	return angle;
}

unsigned char NavCompass::I2CRead(uint8_t i2cAddress, uint8_t address)
{
	char temp;

	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	NAVCOMPASS_I2C.endTransmission();
	NAVCOMPASS_I2C.requestFrom(i2cAddress, (uint8_t) 1);
	temp = NAVCOMPASS_I2C.read();
	NAVCOMPASS_I2C.endTransmission();

	return temp;
}

void NavCompass::I2CBurstRead(uint8_t i2cAddress, uint8_t address, uint8_t *buffer, uint8_t length)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	NAVCOMPASS_I2C.endTransmission();
	NAVCOMPASS_I2C.requestFrom(i2cAddress, (uint8_t) length);
	NAVCOMPASS_I2C.readBytes(buffer, NAVCOMPASS_I2C.available());
	NAVCOMPASS_I2C.endTransmission();
}

void NavCompass::I2CWrite(uint8_t i2cAddress, uint8_t data, uint8_t address)
{
	NAVCOMPASS_I2C.beginTransmission(i2cAddress);
	NAVCOMPASS_I2C.write(address);
	NAVCOMPASS_I2C.write(data);
	NAVCOMPASS_I2C.endTransmission();
}
