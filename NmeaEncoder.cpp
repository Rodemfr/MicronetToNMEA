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

#include "NmeaEncoder.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS 100

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

using namespace std;

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

NmeaEncoder::NmeaEncoder()
{
	memset(&timeStamps, 0, sizeof(timeStamps));
}

NmeaEncoder::~NmeaEncoder()
{
}

bool NmeaEncoder::EncodeMWV_R(NavigationData *micronetData, char *sentence)
{
	bool update = false;

	update |= (micronetData->awa_deg.timeStamp > timeStamps.vwr + MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS);
	update |= (micronetData->aws_kt.timeStamp > timeStamps.vwr + MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS);
	update &= (micronetData->awa_deg.valid && micronetData->aws_kt.valid);

	if (update)
	{
		float absAwa = micronetData->awa_deg.value;
		if (absAwa < 0.0f)
			absAwa += 360.0f;
		sprintf(sentence, "$INMWV,%.1f,R,%.1f,N,A", absAwa, micronetData->aws_kt.value);
		AddNmeaChecksum(sentence);

		timeStamps.vwr = millis();
	}

	return update;
}

bool NmeaEncoder::EncodeMWV_T(NavigationData *micronetData, char *sentence)
{
	bool update = false;

	update |= (micronetData->twa_deg.timeStamp > timeStamps.vwt + MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS);
	update |= (micronetData->tws_kt.timeStamp > timeStamps.vwt + MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS);
	update &= (micronetData->twa_deg.valid && micronetData->tws_kt.valid);

	if (update)
	{
		float absTwa = micronetData->twa_deg.value;
		if (absTwa < 0.0f)
			absTwa += 360.0f;
		sprintf(sentence, "$INMWV,%.1f,T,%.1f,N,A", absTwa, micronetData->tws_kt.value);
		AddNmeaChecksum(sentence);

		timeStamps.vwt = millis();
	}

	return update;
}

bool NmeaEncoder::EncodeDPT(NavigationData *micronetData, char *sentence)
{
	bool update = false;

	update |= (micronetData->dpt_m.timeStamp > timeStamps.dpt + MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS);
	update &= micronetData->dpt_m.valid;

	if (update)
	{
		sprintf(sentence, "$INDPT,%.1f,0.0", micronetData->dpt_m.value);
		AddNmeaChecksum(sentence);

		timeStamps.dpt = millis();
	}

	return update;
}

bool NmeaEncoder::EncodeMTW(NavigationData *micronetData, char *sentence)
{
	bool update = false;

	update |= (micronetData->stp_degc.timeStamp > timeStamps.mtw + MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS);
	update &= micronetData->stp_degc.valid;

	if (update)
	{
		sprintf(sentence, "$INMTW,%.1f,C", micronetData->stp_degc.value);
		AddNmeaChecksum(sentence);

		timeStamps.mtw = millis();
	}

	return update;
}

bool NmeaEncoder::EncodeVLW(NavigationData *micronetData, char *sentence)
{
	bool update = false;

	update |= (micronetData->log_nm.timeStamp > timeStamps.vlw + MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS);
	update |= (micronetData->trip_nm.timeStamp > timeStamps.vlw + MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS);
	update &= (micronetData->log_nm.valid && micronetData->trip_nm.valid);

	if (update)
	{
		sprintf(sentence, "$INVLW,%.1f,N,%.1f,N", micronetData->log_nm.value, micronetData->trip_nm.value);
		AddNmeaChecksum(sentence);

		timeStamps.vlw = millis();
	}

	return update;
}

bool NmeaEncoder::EncodeVHW(NavigationData *micronetData, char *sentence)
{
	bool update = false;

	update |= (micronetData->stw_kt.timeStamp > timeStamps.vhw + MINIMUM_DELAY_BEFORE_SENTENCE_UPDATE_MS);
	update &= micronetData->stw_kt.valid;

	if (update)
	{
		sprintf(sentence, "$INVHW,,T,,M,%.2f,N,,K", micronetData->stw_kt.value);
		AddNmeaChecksum(sentence);

		timeStamps.vhw = millis();
	}

	return update;
}

uint8_t NmeaEncoder::AddNmeaChecksum(char *sentence)
{
	uint8_t crc = 0;
	char crcString[8];
	char *pChar = sentence + 1;

	while (*pChar != 0)
	{
		crc ^= (*pChar);
		pChar++;
	}

	sprintf(crcString, "*%02x%c%c", crc, 13, 10);
	strcat(sentence, crcString);

	return crc;
}
