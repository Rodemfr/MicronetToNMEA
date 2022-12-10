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

#define HEADING_HISTORY_LENGTH 8

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

	void Normalize(vec *a);
	void CrossProduct(vec *a, vec *b, vec *out);
	float vector_dot(vec *a, vec *b);
};

#endif /* NAVCOMPASS_H_ */
