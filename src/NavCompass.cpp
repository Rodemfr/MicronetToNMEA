/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Compass handler                                               *
 * Author:   Ronan Demoment, Dietmar Warning                               *
 *           Heading algorithm heavily based on pololu's code :            *
 *           https://github.com/pololu/lsm303-arduino                      *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021 by Ronan Demoment                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "NavCompass.h"
#include "BoardConfig.h"
#include "Globals.h"
#include "LSM303DLHCDriver.h"
#include "LSM303DLHDriver.h"

#include <cmath>
#include <vector>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                           Static & Globals                              */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

NavCompass::NavCompass() : headingIndex(0), navCompassDetected(false), navCompassDriver(nullptr)
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
    vec accel;
    vec mag;
    vec E;
    vec N;

    vec from = HEADING_AXIS;

    // Get Acceleration and Magnetic data from LSM303
    // Note that we don't care about units of both acceleration and magnetic field since we
    // are only calculating angles.
    navCompassDriver->GetAcceleration(&accel);
    navCompassDriver->GetMagneticField(&mag);

    // Substract calibration offsets from magnetic readings
    mag.x -= gConfiguration.xMagOffset;
    mag.y -= gConfiguration.yMagOffset;
    mag.z -= gConfiguration.zMagOffset;

    // normalize
    Normalize(&accel);
    Normalize(&mag);

    // D X M = E, cross acceleration vector Down with M (magnetic north + inclination) to produce "East"
    CrossProduct(&mag, &accel, &E);
    Normalize(&E);
    // E X D = N, cross "East" with "Down" to produce "North" (parallel to the ground)
    CrossProduct(&accel, &E, &N);
    Normalize(&N);

    // compute heading
    float heading = atan2f(vector_dot(&E, &from), vector_dot(&N, &from)) * 180.0f / PI;

    if (heading < 0)
        heading += 360;

    headingHistory[headingIndex++] = heading;
    if (headingIndex >= HEADING_HISTORY_LENGTH)
    {
        headingIndex = 0;
    }

    bool firstQ     = false;
    bool lastQ      = false;
    bool shiftAngle = false;
    for (int i = 0; i < HEADING_HISTORY_LENGTH; i++)
    {
        if (headingHistory[i] < 90.0)
            firstQ = true;
        if (headingHistory[i] > 270.0)
            lastQ = true;
    }
    shiftAngle = firstQ && lastQ;

    heading = 0.0f;
    for (int i = 0; i < HEADING_HISTORY_LENGTH; i++)
    {
        float value = headingHistory[i];
        if (shiftAngle && (value > 270.0))
            value -= 360.0;
        heading += value;
    }

    return heading / HEADING_HISTORY_LENGTH;
}

void NavCompass::GetMagneticField(float *magX, float *magY, float *magZ)
{
    if (navCompassDetected)
    {
        vec mag;

        navCompassDriver->GetMagneticField(&mag);
        *magX = mag.x;
        *magY = mag.y;
        *magZ = mag.z;
    }
}

void NavCompass::GetAcceleration(float *accX, float *accY, float *accZ)
{
    if (navCompassDetected)
    {
        vec acc;

        navCompassDriver->GetAcceleration(&acc);
        *accX = acc.x;
        *accY = acc.y;
        *accZ = acc.z;
    }
}

void NavCompass::Normalize(vec *a)
{
    float mag = sqrt(vector_dot(a, a));
    a->x /= mag;
    a->y /= mag;
    a->z /= mag;
}

void NavCompass::CrossProduct(vec *a, vec *b, vec *out)
{
    out->x = (a->y * b->z) - (a->z * b->y);
    out->y = (a->z * b->x) - (a->x * b->z);
    out->z = (a->x * b->y) - (a->y * b->x);
}

float NavCompass::vector_dot(vec *a, vec *b)
{
    return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}
