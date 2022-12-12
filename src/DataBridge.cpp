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
{ ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', '\"', ' ', ' ', '%', '&', '\'', ' ', ' ', ' ', '+', ' ', '-', '.', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ' ', '<',
		' ', '>', '?', ' ', 'A', '(', 'C', ')', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		' ', ' ', ' ', ' ', ' ', ' ', 'A', '(', 'C', ')', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
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

DataBridge::DataBridge(MicronetCodec *micronetCodec)
{
	nmeaExtWriteIndex = 0;
	nmeaExtBuffer[0] = 0;
	nmeaGnssWriteIndex = 0;
	nmeaGnssBuffer[0] = 0;
	memset(&nmeaTimeStamps, 0, sizeof(nmeaTimeStamps));
	this->micronetCodec = micronetCodec;

	// Store static link configuration from BoardConfig.h
	navSourceLink = NAV_SOURCE_LINK;
	gnssSourceLink = GNSS_SOURCE_LINK;
	windSourceLink = WIND_SOURCE_LINK;
	depthSourceLink = DEPTH_SOURCE_LINK;
	speedSourceLink = SPEED_SOURCE_LINK;
	voltageSourceLink = VOLTAGE_SOURCE_LINK;
	seaTempSourceLink = SEATEMP_SOURCE_LINK;
	compassSourceLink = COMPASS_SOURCE_LINK;
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
					if (sourceLink == navSourceLink)
					{
						DecodeRMBSentence(nmeaBuffer);
					}
					break;
				case NMEA_ID_RMC:
					if (sourceLink == gnssSourceLink)
					{
						DecodeRMCSentence(nmeaBuffer);
						if (sourceLink != LINK_NMEA_EXT)
						{
							NMEA_EXT.println(nmeaBuffer);
						}
					}
					break;
				case NMEA_ID_GGA:
					if (sourceLink == gnssSourceLink)
					{
						DecodeGGASentence(nmeaBuffer);
						if (sourceLink != LINK_NMEA_EXT)
						{
							NMEA_EXT.println(nmeaBuffer);
						}
					}
					break;
				case NMEA_ID_VTG:
					if (sourceLink == gnssSourceLink)
					{
						DecodeVTGSentence(nmeaBuffer);
						if (sourceLink != LINK_NMEA_EXT)
						{
							NMEA_EXT.println(nmeaBuffer);
						}
					}
					break;
				case NMEA_ID_MWV:
					if (sourceLink == windSourceLink)
					{
						DecodeMWVSentence(nmeaBuffer);
					}
					break;
				case NMEA_ID_DPT:
					if (sourceLink == depthSourceLink)
					{
						DecodeDPTSentence(nmeaBuffer);
					}
					break;
				case NMEA_ID_VHW:
					if (sourceLink == speedSourceLink)
					{
						DecodeVHWSentence(nmeaBuffer);
					}
					break;
				case NMEA_ID_HDG:
					if (sourceLink == compassSourceLink)
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
		while (heading_deg < 0.0f)
			heading_deg += 360.0f;
		while (heading_deg >= 360.0f)
			heading_deg -= 360.0f;
		micronetCodec->navData.magHdg_deg.value = heading_deg;
		micronetCodec->navData.magHdg_deg.valid = true;
		micronetCodec->navData.magHdg_deg.timeStamp = millis();
		EncodeHDG();
	}
}

void DataBridge::UpdateMicronetData()
{
	EncodeMWV_R();
	EncodeMWV_T();
	EncodeDPT();
	EncodeMTW();
	EncodeVLW();
	EncodeVHW();
	EncodeHDG();
	EncodeXDR();
}

bool DataBridge::IsSentenceValid(char *nmeaBuffer)
{
	if (nmeaBuffer[0] != '$')
		return false;

	char *pCs = strrchr(static_cast<char*>(nmeaBuffer), '*') + 1;
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
		micronetCodec->navData.xte_nm.value = value;
		micronetCodec->navData.xte_nm.valid = true;
		micronetCodec->navData.xte_nm.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] == 'R')
		micronetCodec->navData.xte_nm.value = -micronetCodec->navData.xte_nm.value;
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
#if (INVERTED_RMB_WORKAROUND != 1)
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
#endif
	memset(micronetCodec->navData.waypoint.name, ' ', sizeof(micronetCodec->navData.waypoint.name));
	if (sentence[0] != ',')
	{
		// We look for WP1 ID (target)
		uint8_t c;
		uint32_t i;
		for (i = 0; i < sizeof(micronetCodec->navData.waypoint.name); i++)
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
				micronetCodec->navData.waypoint.name[i] = c;
				micronetCodec->navData.waypoint.valid = true;
				micronetCodec->navData.waypoint.timeStamp = millis();
			}
			else
			{
				break;
			}
		}
		micronetCodec->navData.waypoint.nameLength = i;
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
		micronetCodec->navData.dtw_nm.value = value;
		micronetCodec->navData.dtw_nm.valid = true;
		micronetCodec->navData.dtw_nm.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		micronetCodec->navData.btw_deg.value = value;
		micronetCodec->navData.btw_deg.valid = true;
		micronetCodec->navData.btw_deg.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		micronetCodec->navData.vmgwp_kt.value = value;
		micronetCodec->navData.vmgwp_kt.valid = true;
		micronetCodec->navData.vmgwp_kt.timeStamp = millis();
	}
}

void DataBridge::DecodeRMCSentence(char *sentence)
{
	sentence += 7;

	if (sentence[0] != ',')
	{
		micronetCodec->navData.time.hour = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		micronetCodec->navData.time.minute = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		micronetCodec->navData.time.valid = true;
		micronetCodec->navData.time.timeStamp = millis();
	}
	for (int i = 0; i < 8; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sentence[0] != ',')
	{
		micronetCodec->navData.date.day = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		micronetCodec->navData.date.month = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		micronetCodec->navData.date.year = (sentence[4] - '0') * 10 + (sentence[5] - '0');
		micronetCodec->navData.date.valid = true;
		micronetCodec->navData.date.timeStamp = millis();
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
		micronetCodec->navData.latitude_deg.value = degs + mins / 60.0f;
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
		if (sentence[0] == 'S')
			micronetCodec->navData.latitude_deg.value = -micronetCodec->navData.latitude_deg.value;
		micronetCodec->navData.latitude_deg.valid = true;
		micronetCodec->navData.latitude_deg.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 100 + (sentence[1] - '0') * 10 + (sentence[2] - '0');
		sscanf(sentence + 3, "%f,", &mins);
		micronetCodec->navData.longitude_deg.value = degs + mins / 60.0f;
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
		if (sentence[0] == 'W')
			micronetCodec->navData.longitude_deg.value = -micronetCodec->navData.longitude_deg.value;
		micronetCodec->navData.longitude_deg.valid = true;
		micronetCodec->navData.longitude_deg.timeStamp = millis();
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

		micronetCodec->navData.cog_deg.value = value;
		micronetCodec->navData.cog_deg.valid = true;
		micronetCodec->navData.cog_deg.timeStamp = millis();
	}
	for (int i = 0; i < fieldsToSkip; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) == 1)
	{
		micronetCodec->navData.sog_kt.value = value;
		micronetCodec->navData.sog_kt.valid = true;
		micronetCodec->navData.sog_kt.timeStamp = millis();
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
		micronetCodec->navData.awa_deg.value = awa;
		micronetCodec->navData.awa_deg.valid = true;
		micronetCodec->navData.awa_deg.timeStamp = millis();
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

	micronetCodec->navData.aws_kt.value = aws;
	micronetCodec->navData.aws_kt.valid = true;
	micronetCodec->navData.aws_kt.timeStamp = millis();
	micronetCodec->CalculateTrueWind();
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
		micronetCodec->navData.dpt_m.value = depth + value;
		micronetCodec->navData.dpt_m.valid = true;
		micronetCodec->navData.dpt_m.timeStamp = millis();
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
		micronetCodec->navData.magHdg_deg.value = value;
		micronetCodec->navData.magHdg_deg.valid = true;
		micronetCodec->navData.magHdg_deg.timeStamp = millis();
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
		micronetCodec->navData.spd_kt.value = value;
		micronetCodec->navData.spd_kt.valid = true;
		micronetCodec->navData.spd_kt.timeStamp = millis();
	}
}

void DataBridge::DecodeHDGSentence(char *sentence)
{
	float value;

	sentence += 7;

	if (sscanf(sentence, "%f", &value) != 1)
		return;
	while (value < 0)
		value += 360.0f;
	while (value >= 360.0)
		value -= 360.0f;
	micronetCodec->navData.magHdg_deg.value = value;
	micronetCodec->navData.magHdg_deg.valid = true;
	micronetCodec->navData.magHdg_deg.timeStamp = millis();
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

void DataBridge::EncodeMWV_R()
{
	if (WIND_SOURCE_LINK == LINK_MICRONET)
	{
		bool update;

		update = (micronetCodec->navData.awa_deg.timeStamp > nmeaTimeStamps.vwr + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (micronetCodec->navData.aws_kt.timeStamp > nmeaTimeStamps.vwr + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (micronetCodec->navData.awa_deg.valid && micronetCodec->navData.aws_kt.valid);

		if (update)
		{
			char sentence[NMEA_SENTENCE_MAX_LENGTH];
			float absAwa = micronetCodec->navData.awa_deg.value;
			if (absAwa < 0.0f)
				absAwa += 360.0f;
			sprintf(sentence, "$INMWV,%.1f,R,%.1f,N,A", absAwa, micronetCodec->navData.aws_kt.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vwr = millis();
			NMEA_EXT.println(sentence);
		}
	}
}

void DataBridge::EncodeMWV_T()
{
	if (WIND_SOURCE_LINK == LINK_MICRONET)
	{
		bool update;

		update = (micronetCodec->navData.twa_deg.timeStamp > nmeaTimeStamps.vwt + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (micronetCodec->navData.tws_kt.timeStamp > nmeaTimeStamps.vwt + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (micronetCodec->navData.twa_deg.valid && micronetCodec->navData.tws_kt.valid);

		if (update)
		{
			char sentence[NMEA_SENTENCE_MAX_LENGTH];
			float absTwa = micronetCodec->navData.twa_deg.value;
			if (absTwa < 0.0f)
				absTwa += 360.0f;
			sprintf(sentence, "$INMWV,%.1f,T,%.1f,N,A", absTwa, micronetCodec->navData.tws_kt.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vwt = millis();
			NMEA_EXT.println(sentence);
		}
	}
}

void DataBridge::EncodeDPT()
{
	if (DEPTH_SOURCE_LINK == LINK_MICRONET)
	{
		bool update;

		update = (micronetCodec->navData.dpt_m.timeStamp > nmeaTimeStamps.dpt + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && micronetCodec->navData.dpt_m.valid;

		if (update)
		{
			char sentence[NMEA_SENTENCE_MAX_LENGTH];
			sprintf(sentence, "$INDPT,%.1f,%.1f,", micronetCodec->navData.dpt_m.value, micronetCodec->navData.depthOffset_m);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.dpt = millis();
			NMEA_EXT.println(sentence);
		}
	}
}

void DataBridge::EncodeMTW()
{
	if (SEATEMP_SOURCE_LINK == LINK_MICRONET)
	{
		bool update;

		update = (micronetCodec->navData.stp_degc.timeStamp > (nmeaTimeStamps.mtw + NMEA_SENTENCE_MIN_PERIOD_MS));
		update = update && micronetCodec->navData.stp_degc.valid;

		if (update)
		{
			char sentence[NMEA_SENTENCE_MAX_LENGTH];
			sprintf(sentence, "$INMTW,%.1f,C", micronetCodec->navData.stp_degc.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.mtw = millis();
			NMEA_EXT.println(sentence);
		}
	}
}

void DataBridge::EncodeVLW()
{
	if (SPEED_SOURCE_LINK == LINK_MICRONET)
	{
		bool update;

		update = (micronetCodec->navData.log_nm.timeStamp > nmeaTimeStamps.vlw + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (micronetCodec->navData.trip_nm.timeStamp > nmeaTimeStamps.vlw + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (micronetCodec->navData.log_nm.valid && micronetCodec->navData.trip_nm.valid);

		if (update)
		{
			char sentence[NMEA_SENTENCE_MAX_LENGTH];
			sprintf(sentence, "$INVLW,%.1f,N,%.1f,N", micronetCodec->navData.log_nm.value, micronetCodec->navData.trip_nm.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vlw = millis();
			NMEA_EXT.println(sentence);
		}
	}
}

void DataBridge::EncodeVHW()
{
	if (SPEED_SOURCE_LINK == LINK_MICRONET)
	{
		bool update;

		update = (micronetCodec->navData.spd_kt.timeStamp > nmeaTimeStamps.vhw + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update || (micronetCodec->navData.magHdg_deg.timeStamp > nmeaTimeStamps.vhw + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && (micronetCodec->navData.spd_kt.valid || micronetCodec->navData.magHdg_deg.valid);

		if (update)
		{
			char sentence[NMEA_SENTENCE_MAX_LENGTH];
			if ((micronetCodec->navData.magHdg_deg.valid) && (micronetCodec->navData.spd_kt.valid))
			{
				sprintf(sentence, "$INVHW,,T,%.1f,M,%.1f,N,,K", micronetCodec->navData.magHdg_deg.value, micronetCodec->navData.spd_kt.value);
			}
			else if (micronetCodec->navData.magHdg_deg.valid)
			{
				sprintf(sentence, "$INVHW,,T,%.1f,M,,N,,K", micronetCodec->navData.magHdg_deg.value);
			}
			else
			{
				sprintf(sentence, "$INVHW,,T,,M,%.1f,N,,K", micronetCodec->navData.spd_kt.value);
			}
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vhw = millis();
			NMEA_EXT.println(sentence);
		}
	}
}

void DataBridge::EncodeHDG()
{
	if ((COMPASS_SOURCE_LINK == LINK_MICRONET) || (COMPASS_SOURCE_LINK == LINK_COMPASS))
	{
		bool update;

		update = (micronetCodec->navData.magHdg_deg.timeStamp > nmeaTimeStamps.hdg + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && micronetCodec->navData.magHdg_deg.valid;

		if (update)
		{
			char sentence[NMEA_SENTENCE_MAX_LENGTH];
			sprintf(sentence, "$INHDG,%.1f,,,%.1f,%c", micronetCodec->navData.magHdg_deg.value, fabsf(micronetCodec->navData.magneticVariation_deg),
					(micronetCodec->navData.magneticVariation_deg < 0.0f) ? 'W' : 'E');
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.hdg = millis();
			NMEA_EXT.println(sentence);
		}
	}
}

void DataBridge::EncodeXDR()
{
	if (VOLTAGE_SOURCE_LINK == LINK_MICRONET)
	{
		bool update;

		update = (micronetCodec->navData.vcc_v.timeStamp > nmeaTimeStamps.vcc + NMEA_SENTENCE_MIN_PERIOD_MS);
		update = update && micronetCodec->navData.vcc_v.valid;

		if (update)
		{
			char sentence[NMEA_SENTENCE_MAX_LENGTH];
			sprintf(sentence, "$INXDR,U,%.1f,V,BATTERY#0", micronetCodec->navData.vcc_v.value);
			AddNmeaChecksum(sentence);
			nmeaTimeStamps.vcc = millis();
			NMEA_EXT.println(sentence);
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
