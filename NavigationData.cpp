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

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "NavigationData.h"

#include <Arduino.h>
#include <string.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/***************************************************************************/
/*                                Macros                                   */
/***************************************************************************/

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

NavigationData::NavigationData()
{
	stw.valid = false;
	awa.valid = false;
	aws.valid = false;
	twa.valid = false;
	tws.valid = false;
	dpt.valid = false;
	vcc.valid = false;
	log.valid = false;
	trip.valid = false;
	stp.valid = false;
	time.valid = false;
	date.valid = false;
	latitude.valid = false;
	longitude.valid = false;
	cog.valid = false;
	sog.valid = false;

	calibrationUpdated = false;
	waterSpeedFactor_per = 0.0f;
	waterTemperatureOffset_C = 0.0f;
	depthOffset_m = 0.0f;
	windSpeedFactor_per = 0.0f;
	windDirectionOffset_deg = 0.0f;
	headingOffset_deg = 0.0f;
	magneticVariation_deg = 0.0f;
	windShift = 0.0f;
}

NavigationData::~NavigationData()
{
}

void NavigationData::UpdateValidity()
{
	uint32_t currentTime = millis();
	if (awa.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		awa.valid = false;
	}
	if (aws.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		aws.valid = false;
	}
	if (dpt.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		dpt.valid = false;
	}
	if (log.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		log.valid = false;
	}
	if (stp.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		stp.valid = false;
	}
	if (stw.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		stw.valid = false;
	}
	if (trip.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		trip.valid = false;
	}
	if (twa.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		twa.valid = false;
	}
	if (tws.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		tws.valid = false;
	}
	if (vcc.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		vcc.valid = false;
	}
	if (time.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		time.valid = false;
	}
	if (date.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		date.valid = false;
	}
	if (latitude.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		latitude.valid = false;
	}
	if (longitude.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		longitude.valid = false;
	}
	if (cog.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		cog.valid = false;
	}
	if (sog.timeStamp - currentTime > VALIDITY_TIME_MS)
	{
		sog.valid = false;
	}
}
