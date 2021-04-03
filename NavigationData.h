/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Decode data from Micronet devices send it on an NMEA network  *
 * Author:   Ronan Demoment                                                *
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

#ifndef NAVIGATIONDATA_H_
#define NAVIGATIONDATA_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include <stdint.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define VALIDITY_TIME_MS 3000

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

typedef struct
{
	bool valid;
	float value;
	uint32_t timeStamp;
} FloatValue_t;

typedef struct
{
	bool valid;
	uint8_t hour;
	uint8_t minute;
	uint32_t timeStamp;
} TimeValue_t;

typedef struct
{
	bool valid;
	uint8_t day;
	uint8_t month;
	uint8_t year;
	uint32_t timeStamp;
} DateValue_t;

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class NavigationData
{
public:
	NavigationData();
	virtual ~NavigationData();

	void UpdateValidity();

	FloatValue_t stw;
	FloatValue_t awa;
	FloatValue_t aws;
	FloatValue_t twa;
	FloatValue_t tws;
	FloatValue_t dpt;
	FloatValue_t vcc;
	FloatValue_t log;
	FloatValue_t trip;
	FloatValue_t stp;

	TimeValue_t time;
	DateValue_t date;
	FloatValue_t latitude;
	FloatValue_t longitude;
	FloatValue_t cog;
	FloatValue_t sog;

	bool calibrationUpdated;
	float waterSpeedFactor_per;
	float waterTemperatureOffset_C;
	float depthOffset_m;
	float windSpeedFactor_per;
	float windDirectionOffset_deg;
	float headingOffset_deg;
	float magneticVariation_deg;
	float windShift;
};

#endif /* NAVIGATIONDATA_H_ */
