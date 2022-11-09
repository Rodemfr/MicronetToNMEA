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

#define WAYPOINT_NAME_LENGTH  16

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

typedef struct
{
	bool valid;
	uint8_t name[WAYPOINT_NAME_LENGTH];
	uint8_t nameLength;
	uint32_t timeStamp;
} WaypointName_t;

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class NavigationData
{
public:
	NavigationData();
	virtual ~NavigationData();

	void UpdateValidity();

	FloatValue_t spd_kt;
	FloatValue_t awa_deg;
	FloatValue_t aws_kt;
	FloatValue_t twa_deg;
	FloatValue_t tws_kt;
	FloatValue_t dpt_m;
	FloatValue_t vcc_v;
	FloatValue_t log_nm;
	FloatValue_t trip_nm;
	FloatValue_t stp_degc;

	TimeValue_t time;
	DateValue_t date;
	FloatValue_t latitude_deg;
	FloatValue_t longitude_deg;
	FloatValue_t cog_deg;
	FloatValue_t sog_kt;
	FloatValue_t xte_nm;
	FloatValue_t dtw_nm;
	FloatValue_t btw_deg;
	WaypointName_t waypoint;
	FloatValue_t vmgwp_kt;

	FloatValue_t hdg_deg;

	bool calibrationUpdated;
	float waterSpeedFactor_per;
	float waterTemperatureOffset_degc;
	float depthOffset_m;
	float windSpeedFactor_per;
	float windDirectionOffset_deg;
	float headingOffset_deg;
	float magneticVariation_deg;
	float windShift_min;
};

#endif /* NAVIGATIONDATA_H_ */
