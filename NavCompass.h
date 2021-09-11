/*
 * CompassDecoder.h
 *
 *  Created on: 13 juin 2021
 *      Author: Ronan
 */

#ifndef NAVCOMPASS_H_
#define NAVCOMPASS_H_

#include "NavCompassDriver.h"

#include <stdint.h>
#include <string>

using string = std::string;

class NavCompass
{
public:
	NavCompass();
	virtual ~NavCompass();

	bool Init();
	string GetDeviceName();
	float GetHeading();
	void GetMagneticField(float *magX, float* magY, float *magZ);
	void GetAcceleration(float *accX, float* accY, float *accZ);

private:
	float heading;
	bool navCompassDetected;
	NavCompassDriver *navCompassDriver;
};

#endif /* NAVCOMPASS_H_ */
