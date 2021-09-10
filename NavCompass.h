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
	uint8_t accAddr, magAddr;
	uint32_t deviceType;
	float heading;
	float magX, magY, magZ;
	float accX, accY, accZ;

	bool I2CRead(uint8_t i2cAddress, uint8_t address, uint8_t *data);
	bool I2CBurstRead(uint8_t i2cAddress, uint8_t address, uint8_t *buffer, uint8_t length);
	bool I2CWrite(uint8_t i2cAddress, uint8_t data, uint8_t address);
};

#endif /* NAVCOMPASS_H_ */
