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

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

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

NmeaEncoder::NmeaEncoder() :
		INVWRTimeStamp(0)
{
}

NmeaEncoder::~NmeaEncoder()
{
}

bool NmeaEncoder::EncodeINVWR(MicronetData_t *micronetData, char *sentence)
{
	bool update = false;
	char crcString[8];

	update |= (micronetData->awa.timeStamp > INVWRTimeStamp + 100);
	update |= (micronetData->aws.timeStamp > INVWRTimeStamp + 100);
	update &= (micronetData->aws.valid || micronetData->aws.valid);

	if (update)
	{
		// $--VWR,x.x,a,x.x,N,x.x,M,x.x,K*hh<CR><LF>
		sprintf(sentence, "$INVWR,%.1f,%c,%.1f,N,,M,,K", (float)fabs(micronetData->awa.value),
				micronetData->awa.value < 0 ? 'L' : 'R', micronetData->aws.value);
		sprintf(crcString, "*%02x%c%c", (int)NmeaChecksum(sentence), 13, 10);
		strcat(sentence, crcString);

		INVWRTimeStamp = millis();
	}
	return update;
}

// FIXME : Make calculation of true wind with STW
bool NmeaEncoder::EncodeINVWT(MicronetData_t *micronetData, char *sentence)
{
	bool update = false;
	char crcString[8];

	update |= (micronetData->awa.timeStamp > INVWRTimeStamp + 100);
	update |= (micronetData->aws.timeStamp > INVWRTimeStamp + 100);
	update &= (micronetData->aws.valid || micronetData->aws.valid);

	if (update)
	{
		// $--VWR,x.x,a,x.x,N,x.x,M,x.x,K*hh<CR><LF>
		sprintf(sentence, "$INVWT,%.1f,%c,%.1f,N,,M,,K", (float)fabs(micronetData->awa.value),
				micronetData->awa.value < 0 ? 'L' : 'R', micronetData->aws.value);
		sprintf(crcString, "*%02x%c%c", (int)NmeaChecksum(sentence), 13, 10);
		strcat(sentence, crcString);

		INVWTTimeStamp = millis();
	}
	return update;
}

uint8_t NmeaEncoder::NmeaChecksum(char *sentence)
{
	uint8_t crc = 0;
	char *pChar = sentence + 1;

	while (*pChar != 0)
	{
		crc ^= (*pChar);
		pChar++;
	}

	return crc;
}
