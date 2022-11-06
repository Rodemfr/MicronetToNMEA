/*
 * NavCompassDriver.h
 *
 *  Created on: 11 sept. 2021
 *      Author: Ronan
 */

#ifndef NAVCOMPASSDRIVER_H_
#define NAVCOMPASSDRIVER_H_

#include <string>

using string = std::string;

class NavCompassDriver
{
public:
	virtual ~NavCompassDriver() = 0;
	virtual bool Init() = 0;
	virtual string GetDeviceName() = 0;
	virtual void GetMagneticField(float *magX, float* magY, float *magZ) = 0;
	virtual void GetAcceleration(float *accX, float* accY, float *accZ) = 0;
};

#endif /* NAVCOMPASSDRIVER_H_ */
