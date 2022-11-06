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
	spd_kt.valid = false;
	awa_deg.valid = false;
	aws_kt.valid = false;
	twa_deg.valid = false;
	tws_kt.valid = false;
	dpt_m.valid = false;
	vcc_v.valid = false;
	log_nm.valid = false;
	trip_nm.valid = false;
	stp_degc.valid = false;
	time.valid = false;
	date.valid = false;
	latitude_deg.valid = false;
	longitude_deg.valid = false;
	cog_deg.valid = false;
	sog_kt.valid = false;
	xte_nm.valid = false;
	dtw_nm.valid = false;
	btw_deg.valid = false;
	waypoint.valid = false;
	vmgwp_kt.valid = false;
	hdg_deg.valid = false;

	calibrationUpdated = false;
	waterSpeedFactor_per = 0.0f;
	waterTemperatureOffset_degc = 0.0f;
	depthOffset_m = 0.0f;
	windSpeedFactor_per = 0.0f;
	windDirectionOffset_deg = 0.0f;
	headingOffset_deg = 0.0f;
	magneticVariation_deg = 0.0f;
	windShift_min = 0.0f;
}

NavigationData::~NavigationData()
{
}

void NavigationData::UpdateValidity()
{
	uint32_t currentTime = millis();

	if (currentTime - awa_deg.timeStamp > VALIDITY_TIME_FAST_MS)
		awa_deg.valid = false;
	if (currentTime - aws_kt.timeStamp > VALIDITY_TIME_FAST_MS)
		aws_kt.valid = false;
	if (currentTime - dpt_m.timeStamp > VALIDITY_TIME_FAST_MS)
		dpt_m.valid = false;
	if (currentTime - log_nm.timeStamp > VALIDITY_TIME_FAST_MS)
		log_nm.valid = false;
	if (currentTime - stp_degc.timeStamp > VALIDITY_TIME_FAST_MS)
		stp_degc.valid = false;
	if (currentTime - spd_kt.timeStamp > VALIDITY_TIME_FAST_MS)
		spd_kt.valid = false;
	if (currentTime - trip_nm.timeStamp > VALIDITY_TIME_FAST_MS)
		trip_nm.valid = false;
	if (currentTime - twa_deg.timeStamp > VALIDITY_TIME_FAST_MS)
		twa_deg.valid = false;
	if (currentTime - tws_kt.timeStamp > VALIDITY_TIME_FAST_MS)
		tws_kt.valid = false;
	if (currentTime - vcc_v.timeStamp > VALIDITY_TIME_FAST_MS)
		vcc_v.valid = false;
	if (currentTime - time.timeStamp > VALIDITY_TIME_SLOW_MS)
		time.valid = false;
	if (currentTime - date.timeStamp > VALIDITY_TIME_SLOW_MS)
		date.valid = false;
	if (currentTime - latitude_deg.timeStamp > VALIDITY_TIME_SLOW_MS)
		latitude_deg.valid = false;
	if (currentTime - longitude_deg.timeStamp > VALIDITY_TIME_SLOW_MS)
		longitude_deg.valid = false;
	if (currentTime - cog_deg.timeStamp > VALIDITY_TIME_SLOW_MS)
		cog_deg.valid = false;
	if (currentTime - sog_kt.timeStamp > VALIDITY_TIME_SLOW_MS)
		sog_kt.valid = false;
	if (currentTime - xte_nm.timeStamp > VALIDITY_TIME_SLOW_MS)
		xte_nm.valid = false;
	if (currentTime - dtw_nm.timeStamp > VALIDITY_TIME_SLOW_MS)
		dtw_nm.valid = false;
	if (currentTime - btw_deg.timeStamp > VALIDITY_TIME_SLOW_MS)
		btw_deg.valid = false;
	if (currentTime - waypoint.timeStamp > VALIDITY_TIME_SLOW_MS)
		waypoint.valid = false;
	if (currentTime - vmgwp_kt.timeStamp > VALIDITY_TIME_SLOW_MS)
		vmgwp_kt.valid = false;
	if (currentTime - hdg_deg.timeStamp > VALIDITY_TIME_FAST_MS)
		hdg_deg.valid = false;
}
