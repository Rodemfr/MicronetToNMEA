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
} DataValue_t;

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class NavigationData
{
public:
	NavigationData();
	virtual ~NavigationData();

	void UpdateValidity();

	DataValue_t stw;
	DataValue_t awa;
	DataValue_t aws;
	DataValue_t twa;
	DataValue_t tws;
	DataValue_t dpt;
	DataValue_t vcc;
	DataValue_t log;
	DataValue_t trip;
	DataValue_t stp;
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
