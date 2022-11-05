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
#include "LSM303DLHCDriver.h"

NavCompass::NavCompass() :
		headingIndex(0), navCompassDetected(false), navCompassDriver(nullptr)
{
	for (int i = 0; i < HEADING_HISTORY_LENGTH; i++)
	{
		headingHistory[i] = 0.0f;
	}
}

NavCompass::~NavCompass()
{
}

bool NavCompass::Init()
{
	navCompassDetected = false;

	navCompassDriver = new LSM303DLHCDriver();
	if (!navCompassDriver->Init())
	{
		delete navCompassDriver;
		navCompassDriver = new LSM303DLHDriver();
	}

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
	float starboardY, starboardZ;
	float starboardNorm;
	float pStarboard;
	float bowX, bowY, bowZ;
	float bowNorm;
	float pBow;

	// Get Acceleration and Magnetic data from LSM303
	// Note that we don't care about units of both acceleration and magnetic field since we
	// are only calculating angles.
	navCompassDriver->GetAcceleration(&accelX, &accelY, &accelZ);
	navCompassDriver->GetMagneticField(&magX, &magY, &magZ);

	// Substract calibration offsets from magnetic readings
	magX -= gConfiguration.xMagOffset;
	magY -= gConfiguration.yMagOffset;
	magZ -= gConfiguration.zMagOffset;

	// Build starboard axis from Nav Compass X axis & gravity vector
	starboardY = -accelZ;
	starboardZ = accelY;
	starboardNorm = sqrtf(starboardY * starboardY + starboardZ * starboardZ);

	// Build starboard axis from starboard axis & gravity vector
	bowX = (accelY * accelY) + (accelZ * accelZ);
	bowY = accelX * accelY;
	bowZ = accelX * accelZ;
	bowNorm = sqrtf(bowX * bowX + bowY * bowY + bowZ * bowZ);

	// Project magnetic field on bow & starboard axis
	pBow = (magX * bowX + magY * bowY + magZ * bowZ) / bowNorm;
	pStarboard = (magY * starboardY + magZ * starboardZ) / starboardNorm;

	float angle = atan2(-pStarboard, pBow) * 180 / M_PI;
	if (angle < 0)
		angle += 360;

	headingHistory[headingIndex++] = angle;
	if (headingIndex >= HEADING_HISTORY_LENGTH)
	{
		headingIndex = 0;
	}

	bool firstQ = false;
	bool lastQ = false;
	bool shiftAngle = false;
	for (int i = 0; i < HEADING_HISTORY_LENGTH; i++)
	{
		if (headingHistory[i] < 90.0) firstQ = true;
		if (headingHistory[i] > 270.0) lastQ = true;
	}
	shiftAngle = firstQ && lastQ;

	angle = 0.0f;
	for (int i = 0; i < HEADING_HISTORY_LENGTH; i++)
	{
		float value = headingHistory[i];
		if (shiftAngle && (value > 270.0))
			value -= 360.0;
		angle += value;
	}

	return angle / HEADING_HISTORY_LENGTH;
}

void NavCompass::GetMagneticField(float *magX, float *magY, float *magZ)
{
	if (navCompassDetected)
	{
		navCompassDriver->GetMagneticField(magX, magY, magZ);
	}
}

void NavCompass::GetAcceleration(float *accX, float *accY, float *accZ)
{
	if (navCompassDetected)
	{
		navCompassDriver->GetAcceleration(accX, accY, accZ);
	}
}
