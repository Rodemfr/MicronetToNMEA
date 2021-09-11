/*
 * CompassDecoder.cpp
 *
 *  Created on: 13 juin 2021
 *      Author: Ronan
 */

#include "NavCompass.h"
#include "BoardConfig.h"
#include "Globals.h"
#include "LSM303DLHDriver.h"

NavCompass::NavCompass() :
		heading(0), navCompassDetected(false), navCompassDriver(nullptr)
{
}

NavCompass::~NavCompass()
{
}

bool NavCompass::Init()
{
	navCompassDetected = false;

	navCompassDriver = new LSM303DLHDriver();
	if (!navCompassDriver->Init())
	{
		delete navCompassDriver;
		return false;
	}

	navCompassDetected = true;
	return true;
}

string NavCompass::GetDeviceName()
{
	if (navCompassDetected)
	{
		return navCompassDriver->GetDeviceName();
	}

	return string("");
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
	navCompassDriver->GetAcceleration(&accelX, &accelY, &accelZ);
	navCompassDriver->GetMagneticField(&magX, &magY, &magZ);

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

void NavCompass::GetMagneticField(float *magX, float* magY, float *magZ)
{
	if (navCompassDetected)
	{
		navCompassDriver->GetMagneticField(magX, magY, magZ);
	}
}

void NavCompass::GetAcceleration(float *accX, float* accY, float *accZ)
{
	if (navCompassDetected)
	{
		navCompassDriver->GetAcceleration(accX, accY, accZ);
	}
}
