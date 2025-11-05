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
#include "LSM303AGRDriver.h"
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

const Vec3D NavCompass::Axis[6] = {
    {1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0},
};

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
        if (!navCompassDriver->Init())
        {
            delete navCompassDriver;
            navCompassDriver = new LSM303AGRDriver();
            if (!navCompassDriver->Init())
            {
                delete navCompassDriver;
                return false;
            }
        }
    }

    // Compute starboard axis from heading/box axis and down axis
    headingAxis = Axis[gConfiguration.headingAxis];
    downAxis    = Axis[gConfiguration.downAxis];

    previousHeadingAxis = gConfiguration.headingAxis;
    previousDownAxis    = gConfiguration.downAxis;

    CrossProduct(&downAxis, &headingAxis, &starBoardAxis);

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

void NavCompass::GetHeadingAndRoll(float *heading_deg, float *roll_deg)
{
    Vec3D accel;
    Vec3D mag;
    Vec3D E;
    Vec3D N;

    // Compute starboard axis from heading/box axis and down axis
    if ((gConfiguration.headingAxis != previousHeadingAxis) || (gConfiguration.downAxis != previousDownAxis))
    {
        headingAxis = Axis[gConfiguration.headingAxis];
        downAxis    = Axis[gConfiguration.downAxis];

        previousHeadingAxis = gConfiguration.headingAxis;
        previousDownAxis    = gConfiguration.downAxis;

        CrossProduct(&downAxis, &headingAxis, &starBoardAxis);
    }

    // Set heading & roll to default values. These values will only be used if something
    // went wrong in calculations below (i.e. in case of bad measurements from LSM303)
    int tempIndex = headingIndex - 1;
    if (tempIndex < 0)
    {
        tempIndex += HEADING_HISTORY_LENGTH;
    }
    *heading_deg = headingHistory[tempIndex];

    tempIndex = rollIndex - 1;
    if (tempIndex < 0)
    {
        tempIndex += ROLL_HISTORY_LENGTH;
    }
    *roll_deg = rollHistory[tempIndex];

    // Get Acceleration and Magnetic data from LSM303
    // Note that we don't care about units of both acceleration and magnetic field since we
    // are only calculating angles.
    navCompassDriver->GetAcceleration(&accel);
    navCompassDriver->GetMagneticField(&mag);

    // Substract calibration offsets from magnetic readings
    mag.x -= gConfiguration.xMagOffset;
    mag.y -= gConfiguration.yMagOffset;
    mag.z -= gConfiguration.zMagOffset;

    // Check for invalid measurements
    if (isZero(&accel) || isZero(&mag))
    {
        return;
    }

    // normalize
    Normalize(&accel);
    Normalize(&mag);

    // D X M = E, cross acceleration vector Down with M (magnetic north + inclination) to produce "East"
    CrossProduct(&mag, &accel, &E);
    // E X D = N, cross "East" with "Down" to produce "North" (parallel to the ground plane)
    CrossProduct(&accel, &E, &N);

    // Check for invalid vectors
    if (isZero(&E) || isZero(&N))
    {
        return;
    }

    Normalize(&E);
    Normalize(&N);

    // compute heading
    float heading = atan2f(VectorDot(&E, &headingAxis), VectorDot(&N, &headingAxis)) * 180.0f / PI;

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

    *heading_deg = heading / HEADING_HISTORY_LENGTH;

    // compute roll angle
    float roll               = atan2f(-VectorDot(&accel, &starBoardAxis), -VectorDot(&accel, &downAxis)) * 180.0f / PI;
    rollHistory[rollIndex++] = roll;
    if (rollIndex >= ROLL_HISTORY_LENGTH)
    {
        rollIndex = 0;
    }

    roll = 0.0f;
    for (int i = 0; i < ROLL_HISTORY_LENGTH; i++)
    {
        roll += rollHistory[i];
    }

    *roll_deg = roll / ROLL_HISTORY_LENGTH;
}

void NavCompass::GetMagneticField(float *magX, float *magY, float *magZ)
{
    if (navCompassDetected)
    {
        Vec3D mag;

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
        Vec3D acc;

        navCompassDriver->GetAcceleration(&acc);
        *accX = acc.x;
        *accY = acc.y;
        *accZ = acc.z;
    }
}

bool NavCompass::isZero(Vec3D *a)
{
    return ((a->x == 0) && (a->y == 0) && (a->z == 0));
}

void NavCompass::Normalize(Vec3D *a)
{
    float mag = sqrtf(VectorDot(a, a));
    a->x /= mag;
    a->y /= mag;
    a->z /= mag;
}

void NavCompass::CrossProduct(Vec3D *a, Vec3D *b, Vec3D *out)
{
    out->x = (a->y * b->z) - (a->z * b->y);
    out->y = (a->z * b->x) - (a->x * b->z);
    out->z = (a->x * b->y) - (a->y * b->x);
}

float NavCompass::VectorDot(Vec3D *a, Vec3D *b)
{
    return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}
