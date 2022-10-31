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
#include "Globals.h"

#include <Arduino.h>
#include <string.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

const uint8_t DataBridge::asciiTable[128] =
{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\"', ' ', ' ', '%', '&', '\'', ' ', ' ', ' ', '+', ' ', '-', '.', '/', '0',
		'1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ' ', '<', ' ', '>', '?', ' ', 'A', '(', 'C', ')', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ', ' ', ' ', ' ', ' ', ' ',
		'A', '(', 'C', ')', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', ' ', ' ', ' ', ' ', ' ' };

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

DataBridge::DataBridge()
{
	nmeaExtWriteIndex = 0;
	nmeaExtBuffer[0] = 0;
	nmeaGnssWriteIndex = 0;
	nmeaGnssBuffer[0] = 0;
	memset(&nmeaTimeStamps, 0, sizeof(nmeaTimeStamps));
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
		nmeaBuffer[*nmeaWriteIndex] = 0;

		if ((*nmeaWriteIndex >= 10) && (*nmeaWriteIndex < NMEA_SENTENCE_MAX_LENGTH - 1))
		{
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
						NMEA_OUT.println(nmeaBuffer);
					}
					break;
				case NMEA_ID_GGA:
					if (sourceLink == GNSS_SOURCE_LINK)
					{
						DecodeGGASentence(nmeaBuffer);
						NMEA_OUT.println(nmeaBuffer);
					}
					break;
				case NMEA_ID_VTG:
					if (sourceLink == GNSS_SOURCE_LINK)
					{
						DecodeVTGSentence(nmeaBuffer);
						NMEA_OUT.println(nmeaBuffer);
					}
					break;
				case NMEA_ID_MWV:
					if (sourceLink == WIND_SOURCE_LINK)
					{
						DecodeMWVSentence(nmeaBuffer);
					}
					break;
				case NMEA_ID_DPT:
					if (sourceLink == DEPTH_SOURCE_LINK)
					{
						DecodeDPTSentence(nmeaBuffer);
					}
					break;
				case NMEA_ID_VHW:
					if (sourceLink == SPEED_SOURCE_LINK)
					{
						DecodeVHWSentence(nmeaBuffer);
					}
					break;
				case NMEA_ID_HDG:
					if (sourceLink == COMPASS_SOURCE_LINK)
					{
						DecodeHDGSentence(nmeaBuffer);
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

	nmeaBuffer[*nmeaWriteIndex] = c;
	(*nmeaWriteIndex)++;

	if (*nmeaWriteIndex >= NMEA_SENTENCE_MAX_LENGTH)
	{
		nmeaBuffer[0] = 0;
		*nmeaWriteIndex = 0;
		return;
	}
}

void DataBridge::UpdateCompassData(float heading_deg)
{
	if (COMPASS_SOURCE_LINK == LINK_COMPASS)
	{
		gNavData.hdg_deg.value = heading_deg;
		gNavData.hdg_deg.valid = true;
		gNavData.hdg_deg.timeStamp = millis();
		EncodeHDG(&gNavData);
	}
}

void DataBridge::UpdateMicronetData()
{
	EncodeMWV_R(&gNavData);
	EncodeMWV_T(&gNavData);
	EncodeDPT(&gNavData);
	EncodeMTW(&gNavData);
	EncodeVLW(&gNavData);
	EncodeVHW(&gNavData);
	EncodeHDG(&gNavData);
	EncodeXDR(&gNavData);
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
	case 0x4D5756:
		// MWV sentence
		nmeaSentence = NMEA_ID_MWV;
		break;
	case 0x445054:
		// DPT sentence
		nmeaSentence = NMEA_ID_DPT;
		break;
	case 0x564857:
		// DPT sentence
		nmeaSentence = NMEA_ID_VHW;
		break;
	case 0x484447:
		// HDG sentence
		nmeaSentence = NMEA_ID_HDG;
		break;
	}

	return nmeaSentence;
}

void DataBridge::DecodeRMBSentence(char *sentence)
{
	float value;

	sentence += 7;

	if (sentence[0] != 'A')
	{
		return;
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		gNavData.xte_nm.value = value;
		gNavData.xte_nm.valid = true;
		gNavData.xte_nm.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] == 'R')
		gNavData.xte_nm.value = -gNavData.xte_nm.value;
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
#if (INVERTED_RMB_WORKAROUND != 1)
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
#endif
	memset(gNavData.waypoint.name, ' ', sizeof(gNavData.waypoint.name));
	if (sentence[0] != ',')
	{
		// We look for WP1 ID (target)
		uint8_t c;
		for (uint32_t i = 0; i < sizeof(gNavData.waypoint.name); i++)
		{
			if (sentence[i] != ',')
			{
				c = sentence[i];
				if (c < 128)
				{
					c = asciiTable[(int32_t) sentence[i]];
				}
				else
				{
					c = ' ';
				}
				gNavData.waypoint.name[i] = c;
				gNavData.waypoint.valid = true;
				gNavData.waypoint.timeStamp = millis();
			}
			else
			{
				break;
			}
		}
	}
#if (INVERTED_RMB_WORKAROUND != 1)
	for (int i = 0; i < 5; i++)
#else
	for (int i = 0; i < 6; i++)
#endif
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) == 1)
	{
		gNavData.dtw_nm.value = value;
		gNavData.dtw_nm.valid = true;
		gNavData.dtw_nm.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		gNavData.btw_deg.value = value;
		gNavData.btw_deg.valid = true;
		gNavData.btw_deg.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		gNavData.vmgwp_kt.value = value;
		gNavData.vmgwp_kt.valid = true;
		gNavData.vmgwp_kt.timeStamp = millis();
	}
}

void DataBridge::DecodeRMCSentence(char *sentence)
{
	sentence += 7;

	if (sentence[0] != ',')
	{
		gNavData.time.hour = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		gNavData.time.minute = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		gNavData.time.valid = true;
		gNavData.time.timeStamp = millis();
	}
	for (int i = 0; i < 8; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sentence[0] != ',')
	{
		gNavData.date.day = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		gNavData.date.month = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		gNavData.date.year = (sentence[4] - '0') * 10 + (sentence[5] - '0');
		gNavData.date.valid = true;
		gNavData.date.timeStamp = millis();
	}
}

void DataBridge::DecodeGGASentence(char *sentence)
{
	float degs, mins;

	sentence += 7;

	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;

	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		sscanf(sentence + 2, "%f,", &mins);
		gNavData.latitude_deg.value = degs + mins / 60.0f;
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
		if (sentence[0] == 'S')
			gNavData.latitude_deg.value = -gNavData.latitude_deg.value;
		gNavData.latitude_deg.valid = true;
		gNavData.latitude_deg.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 100 + (sentence[1] - '0') * 10 + (sentence[2] - '0');
		sscanf(sentence + 3, "%f,", &mins);
		gNavData.longitude_deg.value = degs + mins / 60.0f;
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
		if (sentence[0] == 'W')
			gNavData.longitude_deg.value = -gNavData.longitude_deg.value;
		gNavData.longitude_deg.valid = true;
		gNavData.longitude_deg.timeStamp = millis();
	}
}

void DataBridge::DecodeVTGSentence(char *sentence)
{
	float value;
	int comaCount = 0;
	char *pChar = sentence;
	int fieldsToSkip;

	sentence += 7;

	// Here we check which version of VTG sentence we received
	// older devices might send a sentence without the T, M and N characters
	while ((pChar = strchr(pChar + 1, ',')) != nullptr)
	{
		comaCount++;
	}

	if (comaCount == 4)
	{
		// Version without T, M & N
		fieldsToSkip = 2;
	}
	else
	{
		// Correct version
		fieldsToSkip = 4;
	}

	if (sscanf(sentence, "%f", &value) == 1)
	{
		if (value < 0)
			value += 360.0f;

		gNavData.cog_deg.value = value;
		gNavData.cog_deg.valid = true;
		gNavData.cog_deg.timeStamp = millis();
	}
	for (int i = 0; i < fieldsToSkip; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) == 1)
	{
		gNavData.sog_kt.value = value;
		gNavData.sog_kt.valid = true;
		gNavData.sog_kt.timeStamp = millis();
	}
}

void DataBridge::DecodeMWVSentence(char *sentence)
{
	float value;
	float awa = -9999.0;
	float aws;

	sentence += 7;

	if (sscanf(sentence, "%f", &value) == 1)
	{
		awa = value;
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] != 'R')
	{
		return;
	}
	if (awa > -9000.0)
	{
		if (awa > 180.0)
			awa -= 360.0f;
		gNavData.awa_deg.value = awa;
		gNavData.awa_deg.valid = true;
		gNavData.awa_deg.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		aws = value;
	}
	else
		return;
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	switch (sentence[0])
	{
	case 'M':
		aws *= 1.943844;
		break;
	case 'K':
		aws *= 0.5399568;
		break;
	case 'N':
		break;
	default:
		return;
	}

	gNavData.aws_kt.value = aws;
	gNavData.aws_kt.valid = true;
	gNavData.aws_kt.timeStamp = millis();
	gMicronetCodec.CalculateTrueWind(&gNavData);
}

void DataBridge::DecodeDPTSentence(char *sentence)
{
	float value;
	float depth;

	sentence += 7;

	if (sscanf(sentence, "%f", &value) == 1)
	{
		depth = value;
	}
	else
		return;
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		gNavData.dpt_m.value = depth + value;
		gNavData.dpt_m.valid = true;
		gNavData.dpt_m.timeStamp = millis();
	}
}

void DataBridge::DecodeVHWSentence(char *sentence)
{
	float value;

	sentence += 7;

	for (int i = 0; i < 2; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) != 1)
		return;
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] == 'M')
	{
		if (value < 0)
			value += 360.0f;
		gNavData.hdg_deg.value = value;
		gNavData.hdg_deg.valid = true;
		gNavData.hdg_deg.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) != 1)
		return;
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] == 'N')
	{
		gNavData.spd_kt.value = value;
		gNavData.spd_kt.valid = true;
		gNavData.spd_kt.timeStamp = millis();
	}
}

void DataBridge::DecodeHDGSentence(char *sentence)
{
	float value;

	sentence += 7;

	if (sscanf(sentence, "%f", &value) != 1)
		return;
	if (value < 0)
		value += 360.0f;
	gNavData.hdg_deg.value = value;
	gNavData.hdg_deg.valid = true;
	gNavData.hdg_deg.timeStamp = millis();
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

void DataBridge::EncodeMWV_R(NavigationData *micronetData)
{
	char sentence[NMEA_SENTENCE_MAX_LENGTH];
	bool update;

	if (WIND_SOURCE_LINK == LINK_MICRONET)
	{
		update = (gNavData.awa_deg.timeStamp > nmeaTimeStamps.vwr + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (gNavData.aws_kt.timeStamp > nmeaTimeStamps.vwr + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (gNavData.awa_deg.valid && gNavData.aws_kt.valid);

		if (update)
		{
			float absAwa = gNavData.awa_deg.value;
			if (absAwa < 0.0f)
				absAwa += 360.0f;
			sprintf(sentence, "$INMWV,%.1f,R,%.1f,N,A", absAwa, gNavData.aws_kt.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vwr = millis();
			NMEA_OUT.println(sentence);
		}
	}
}

void DataBridge::EncodeMWV_T(NavigationData *micronetData)
{
	char sentence[NMEA_SENTENCE_MAX_LENGTH];
	bool update;

	if (WIND_SOURCE_LINK == LINK_MICRONET)
	{
		update = (gNavData.twa_deg.timeStamp > nmeaTimeStamps.vwt + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (gNavData.tws_kt.timeStamp > nmeaTimeStamps.vwt + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (gNavData.twa_deg.valid && gNavData.tws_kt.valid);

		if (update)
		{
			float absTwa = gNavData.twa_deg.value;
			if (absTwa < 0.0f)
				absTwa += 360.0f;
			sprintf(sentence, "$INMWV,%.1f,T,%.1f,N,A", absTwa, gNavData.tws_kt.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vwt = millis();
			NMEA_OUT.println(sentence);
		}
	}
}

void DataBridge::EncodeDPT(NavigationData *micronetData)
{
	char sentence[NMEA_SENTENCE_MAX_LENGTH];
	bool update;

	if (DEPTH_SOURCE_LINK == LINK_MICRONET)
	{
		update = (gNavData.dpt_m.timeStamp > nmeaTimeStamps.dpt + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && gNavData.dpt_m.valid;

		if (update)
		{
			sprintf(sentence, "$INDPT,%.1f,0.0", gNavData.dpt_m.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.dpt = millis();
			NMEA_OUT.println(sentence);
		}
	}
}

void DataBridge::EncodeMTW(NavigationData *micronetData)
{
	char sentence[NMEA_SENTENCE_MAX_LENGTH];
	bool update;

	if (SEATEMP_SOURCE_LINK == LINK_MICRONET)
	{
		update = (gNavData.stp_degc.timeStamp > (nmeaTimeStamps.mtw + NMEA_SENTENCE_MIN_PERIOD_MS));
		update = update && gNavData.stp_degc.valid;

		if (update)
		{
			sprintf(sentence, "$INMTW,%.1f,C", gNavData.stp_degc.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.mtw = millis();
			NMEA_OUT.println(sentence);
		}
	}
}

void DataBridge::EncodeVLW(NavigationData *micronetData)
{
	char sentence[NMEA_SENTENCE_MAX_LENGTH];
	bool update;

	if (SPEED_SOURCE_LINK == LINK_MICRONET)
	{
		update = (gNavData.log_nm.timeStamp > nmeaTimeStamps.vlw + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (gNavData.trip_nm.timeStamp > nmeaTimeStamps.vlw + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (gNavData.log_nm.valid && gNavData.trip_nm.valid);

		if (update)
		{
			sprintf(sentence, "$INVLW,%.1f,N,%.1f,N", gNavData.log_nm.value, gNavData.trip_nm.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vlw = millis();
			NMEA_OUT.println(sentence);
		}
	}
}

void DataBridge::EncodeVHW(NavigationData *micronetData)
{
	char sentence[NMEA_SENTENCE_MAX_LENGTH];
	bool update;

	if (SPEED_SOURCE_LINK == LINK_MICRONET)
	{
		update = (gNavData.spd_kt.timeStamp > nmeaTimeStamps.vhw + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update || (gNavData.hdg_deg.timeStamp > nmeaTimeStamps.vhw + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (gNavData.spd_kt.valid || gNavData.hdg_deg.valid);

		if (update)
		{
			if ((gNavData.hdg_deg.valid) && (gNavData.spd_kt.valid))
			{
				sprintf(sentence, "$INVHW,,T,%.0f,M,%.2f,N,,K", gNavData.hdg_deg.value, gNavData.spd_kt.value);
			}
			else if (gNavData.hdg_deg.valid)
			{
				sprintf(sentence, "$INVHW,,T,%.0f,M,,N,,K", gNavData.hdg_deg.value);
			}
			else
			{
				sprintf(sentence, "$INVHW,,T,,M,%.2f,N,,K", gNavData.spd_kt.value);
			}
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vhw = millis();
			NMEA_OUT.println(sentence);
		}
	}
}

void DataBridge::EncodeHDG(NavigationData *micronetData)
{
	char sentence[NMEA_SENTENCE_MAX_LENGTH];
	bool update;

	if ((COMPASS_SOURCE_LINK == LINK_MICRONET) || (COMPASS_SOURCE_LINK == LINK_COMPASS))
	{
		update = (gNavData.hdg_deg.timeStamp > nmeaTimeStamps.hdg + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && gNavData.hdg_deg.valid;

		if (update)
		{
			sprintf(sentence, "$INHDG,%.0f,,,,", gNavData.hdg_deg.value + gConfiguration.headingOffset_deg);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.hdg = millis();
			NMEA_OUT.println(sentence);
		}
	}
}

void DataBridge::EncodeXDR(NavigationData *micronetData)
{
	char sentence[NMEA_SENTENCE_MAX_LENGTH];
	bool update;

	if (VOLTAGE_SOURCE_LINK == LINK_MICRONET)
	{
		update = (gNavData.vcc_v.timeStamp > nmeaTimeStamps.vcc + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && gNavData.vcc_v.valid;

		if (update)
		{
			sprintf(sentence, "$INXDR,U,%.1f,V,BATTERY#0", gNavData.vcc_v.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vcc = millis();
			NMEA_OUT.println(sentence);
		}
	}
}

uint8_t DataBridge::AddNmeaChecksum(char *sentence)
{
	uint8_t crc = 0;
	char crcString[8];
	char *pChar = sentence + 1;

	while (*pChar != 0)
	{
		crc ^= (*pChar);
		pChar++;
	}

	sprintf(crcString, "*%02x", crc);
	strcat(sentence, crcString);

	return crc;
}
