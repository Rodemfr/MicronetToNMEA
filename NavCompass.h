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

#define HEADING_HISTORY_LENGTH 12

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
	float headingHistory[HEADING_HISTORY_LENGTH];
	uint32_t headingIndex;
	bool navCompassDetected;
	NavCompassDriver *navCompassDriver;
};

#endif /* NAVCOMPASS_H_ */
