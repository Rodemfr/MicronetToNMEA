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

#include "BoardConfig.h"
#include "DataBridge.h"

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

DataBridge::DataBridge(NavigationData *navData)
{
	this->micronetData = navData;
	nmeaExtWriteIndex = 0;
	nmeaExtBuffer[0] = 0;
	nmeaGnssWriteIndex = 0;
	nmeaGnssBuffer[0] = 0;
}

DataBridge::~DataBridge()
{
}

void DataBridge::PushNmeaChar(char c, LinkId_t sourceLink)
{
	char *nmeaBuffer = nullptr;
	int *nmeaWriteIndex = 0;

	switch (sourceLink)
	{
	case LINK_NMEA_EXT:
		nmeaBuffer = nmeaExtBuffer;
		nmeaWriteIndex = &nmeaExtWriteIndex;
		break;
	case LINK_NMEA_GNSS:
		nmeaBuffer = nmeaGnssBuffer;
		nmeaWriteIndex = &nmeaGnssWriteIndex;
		break;
	default:
		return;
	}

	if ((nmeaBuffer[0] != '$') || (c == '$'))
	{
		nmeaBuffer[0] = c;
		*nmeaWriteIndex = 1;
		return;
	}

	if (c == 13)
	{
		if ((*nmeaWriteIndex >= 10) && (*nmeaWriteIndex < NMEA_SENTENCE_MAX_LENGTH - 1))
		{
			nmeaBuffer[*nmeaWriteIndex] = 0;

			if (IsSentenceValid(nmeaBuffer))
			{
				NmeaId_t sId = SentenceId(nmeaBuffer);

				switch (sId)
				{
				case NMEA_ID_RMB:
					if (sourceLink == NAV_SOURCE_LINK)
					{
						DecodeRMBSentence(nmeaBuffer);
					}
					break;
				case NMEA_ID_RMC:
					if (sourceLink == GNSS_SOURCE_LINK)
					{
						DecodeRMCSentence(nmeaBuffer);
						if (sourceLink != LINK_NMEA_EXT)
						{
							NMEA_OUT.print(nmeaBuffer);
						}
					}
					break;
				case NMEA_ID_GGA:
					if (sourceLink == GNSS_SOURCE_LINK)
					{
						DecodeRMCSentence(nmeaBuffer);
						if (sourceLink != LINK_NMEA_EXT)
						{
							NMEA_OUT.print(nmeaBuffer);
						}
					}
					break;
				case NMEA_ID_VTG:
					if (sourceLink == GNSS_SOURCE_LINK)
					{
						DecodeRMCSentence(nmeaBuffer);
						if (sourceLink != LINK_NMEA_EXT)
						{
							NMEA_OUT.print(nmeaBuffer);
						}
					}
					break;
				default:
					break;
				}
			}
		}
		else
		{
			nmeaBuffer[0] = 0;
			*nmeaWriteIndex = 0;
			return;
		}
	}

	nmeaBuffer[*nmeaWriteIndex++] = c;

	if (*nmeaWriteIndex >= NMEA_SENTENCE_MAX_LENGTH)
	{
		nmeaBuffer[0] = 0;
		*nmeaWriteIndex = 0;
		return;
	}
}

bool DataBridge::IsSentenceValid(char *nmeaBuffer)
{
	if (nmeaBuffer[0] != '$')
		return false;

	char *pCs = strrchr((char*) nmeaBuffer, '*') + 1;
	if (pCs == nullptr)
		return false;
	int16_t Cs = (NibbleValue(pCs[0]) << 4) | NibbleValue(pCs[1]);
	if (Cs < 0)
		return false;

	uint8_t crc = 0;
	for (char *pC = nmeaBuffer + 1; pC < (pCs - 1); pC++)
	{
		crc = crc ^ (*pC);
	}

	if (crc != Cs)
		return false;

	return true;
}

NmeaId_t DataBridge::SentenceId(char *nmeaBuffer)
{
	uint32_t sId = ((uint8_t) nmeaBuffer[3]) << 16;
	sId |= ((uint8_t) nmeaBuffer[4]) << 8;
	sId |= ((uint8_t) nmeaBuffer[5]);

	NmeaId_t nmeaSentence = NMEA_ID_UNKNOWN;

	switch (sId)
	{
	case 0x524D42:
		// RMB sentence
		nmeaSentence = NMEA_ID_RMB;
		break;
	case 0x524D43:
		// RMC sentence
		nmeaSentence = NMEA_ID_RMC;
		break;
	case 0x474741:
		// GGA sentence
		nmeaSentence = NMEA_ID_GGA;
		break;
	case 0x565447:
		// VTG sentence
		nmeaSentence = NMEA_ID_VTG;
		break;
	}

	return nmeaSentence;
}

void DataBridge::DecodeRMBSentence(char *sentence)
{
	float value;

	if (sentence[0] != 'A')
	{
		return;
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		micronetData->xte_nm.value = value;
		micronetData->xte_nm.valid = true;
		micronetData->xte_nm.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] == 'R')
		micronetData->xte_nm.value = -micronetData->xte_nm.value;
	for (int i = 0; i < 7; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) == 1)
	{
		micronetData->dtw_nm.value = value;
		micronetData->dtw_nm.valid = true;
		micronetData->dtw_nm.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		micronetData->btw_deg.value = value;
		micronetData->btw_deg.valid = true;
		micronetData->btw_deg.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		micronetData->vmc_kt.value = value;
		micronetData->vmc_kt.valid = true;
		micronetData->vmc_kt.timeStamp = millis();
	}
}

void DataBridge::DecodeRMCSentence(char *sentence)
{
	if (sentence[0] != ',')
	{
		micronetData->time.hour = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		micronetData->time.minute = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		micronetData->time.valid = true;
		micronetData->time.timeStamp = millis();
	}
	for (int i = 0; i < 8; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sentence[0] != ',')
	{
		micronetData->date.day = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		micronetData->date.month = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		micronetData->date.year = (sentence[4] - '0') * 10 + (sentence[5] - '0');
		micronetData->date.valid = true;
		micronetData->date.timeStamp = millis();
	}
}

void DataBridge::DecodeGGASentence(char *sentence)
{
	float degs, mins;

	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;

	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		sscanf(sentence + 2, "%f,", &mins);
		micronetData->latitude_deg.value = degs + mins / 60.0f;
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
		if (sentence[0] == 'S')
			micronetData->latitude_deg.value = -micronetData->latitude_deg.value;
		micronetData->latitude_deg.valid = true;
		micronetData->latitude_deg.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 100 + (sentence[1] - '0') * 10 + (sentence[2] - '0');
		sscanf(sentence + 3, "%f,", &mins);
		micronetData->longitude_deg.value = degs + mins / 60.0f;
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
		if (sentence[0] == 'W')
			micronetData->longitude_deg.value = -micronetData->longitude_deg.value;
		micronetData->longitude_deg.valid = true;
		micronetData->longitude_deg.timeStamp = millis();
	}
}

void DataBridge::DecodeVTGSentence(char *sentence)
{
	float value;

	if (sscanf(sentence, "%f", &value) == 1)
	{
		micronetData->cog_deg.value = value;
		micronetData->cog_deg.valid = true;
		micronetData->cog_deg.timeStamp = millis();
	}
	for (int i = 0; i < 4; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) == 1)
	{
		micronetData->sog_kt.value = value;
		micronetData->sog_kt.valid = true;
		micronetData->sog_kt.timeStamp = millis();
	}
}

int16_t DataBridge::NibbleValue(char c)
{
	if ((c >= '0') && (c <= '9'))
	{
		return (c - '0');
	}
	else if ((c >= 'A') && (c <= 'F'))
	{
		return (c - 'A') + 0x0a;
	}
	else if ((c >= 'a') && (c <= 'f'))
	{
		return (c - 'a') + 0x0a;
	}

	return -1;
}
