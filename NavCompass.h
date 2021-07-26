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
public:
	NavCompass();
	virtual ~NavCompass();

	bool Init();
	float GetHeading();
	void GetMagneticField(float *magX, float* magY, float *magZ);
	void GetAcceleration(float *accX, float* accY, float *accZ);

private:
	float heading;
	float magX, magY, magZ;
	float accX, accY, accZ;

	unsigned char I2CRead(uint8_t i2cAddress, uint8_t address);
	void I2CBurstRead(uint8_t i2cAddress, uint8_t address, uint8_t *buffer, uint8_t length);
	void I2CWrite(uint8_t i2cAddress, uint8_t data, uint8_t address);
};

#endif /* NAVCOMPASS_H_ */
