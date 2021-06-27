/*
 * CompassDecoder.h
 *
 *  Created on: 13 juin 2021
 *      Author: Ronan
 */

#ifndef NAVCOMPASS_H_
#define NAVCOMPASS_H_

#include <stdint.h>

class NavCompass
{
private:
	float heading;
	float xOffset, yOffset, zOffset;
	float magX, magY, magZ;
	float accX, accY, accZ;

public:
	NavCompass();
	virtual ~NavCompass();

	bool Init();
	bool IsMagReady();
	void SetMagneticOffset(float xOffset, float yOffset, float zOffset);
	void GetMagneticField(float *magX, float* magY, float *magZ);
	void GetAcceleration(float *accX, float* accY, float *accZ);
	float GetHeading();
private:
	unsigned char I2CRead(uint8_t i2cAddress, uint8_t address);
	void I2CBurstRead(uint8_t i2cAddress, uint8_t address, uint8_t *buffer, uint8_t length);
	void I2CWrite(uint8_t i2cAddress, uint8_t data, uint8_t address);
};

#endif /* NAVCOMPASS_H_ */
