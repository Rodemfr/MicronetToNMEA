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

#ifndef NAVCOMPASS_H_
#define NAVCOMPASS_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "NavCompassDriver.h"

#include <stdint.h>
#include <string>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define HEADING_HISTORY_LENGTH 15
#define ROLL_HISTORY_LENGTH    15

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

using string = std::string;

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class NavCompass
{
  public:
    NavCompass();
    virtual ~NavCompass();

    bool   Init();
    string GetDeviceName();
    void   GetHeadingAndRoll(float *heading_deg, float *roll_deg);
    void   GetMagneticField(float *magX, float *magY, float *magZ);
    void   GetAcceleration(float *accX, float *accY, float *accZ);

  private:
    float             headingHistory[HEADING_HISTORY_LENGTH];
    uint32_t          headingIndex;
    float             rollHistory[ROLL_HISTORY_LENGTH];
    uint32_t          rollIndex;
    bool              navCompassDetected;
    NavCompassDriver *navCompassDriver;
    Vec3D             headingAxis;
    Vec3D             downAxis;
    Vec3D             starBoardAxis;

    void  Normalize(Vec3D *a);
    void  CrossProduct(Vec3D *a, Vec3D *b, Vec3D *out);
    float VectorDot(Vec3D *a, Vec3D *b);
};

#endif /* NAVCOMPASS_H_ */
