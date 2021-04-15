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

#include "NmeaDecoder.h"

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

NmeaDecoder::NmeaDecoder()
{
	writeIndex = 0;
	sentenceWriteIndex = 0;
	serialBuffer[0] = 0;
}

NmeaDecoder::~NmeaDecoder()
{
}

void NmeaDecoder::PushChar(char c, NavigationData *navData)
{
	if ((serialBuffer[0] != '$') || (c == '$'))
	{
		serialBuffer[0] = c;
		writeIndex = 1;
		return;
	}

	if (c == 13)
	{
		if ((writeIndex >= 10)
				&& (sentenceWriteIndex < NMEA_SENTENCE_HISTORY_SIZE))
		{
			memcpy(sentenceBuffer[sentenceWriteIndex], serialBuffer,
					writeIndex);
			sentenceBuffer[sentenceWriteIndex][writeIndex] = 0;
			DecodeSentence(sentenceWriteIndex, navData);
			sentenceWriteIndex++;

		}
		else
		{
			serialBuffer[0] = 0;
			writeIndex = 0;
			return;
		}
	}

	serialBuffer[writeIndex++] = c;

	if (writeIndex >= NMEA_SENTENCE_MAX_LENGTH)
	{
		serialBuffer[0] = 0;
		writeIndex = 0;
		return;
	}
}

int NmeaDecoder::GetNbSentences()
{
	return sentenceWriteIndex;
}

const char* NmeaDecoder::GetSentence(int i)
{
	return sentenceBuffer[i];
}

void NmeaDecoder::resetSentences()
{
	sentenceWriteIndex = 0;
}

void NmeaDecoder::DecodeSentence(int sentenceIndex, NavigationData *navData)
{
	if (sentenceBuffer[sentenceIndex][0] != '$')
		return;

	char *pCs = strrchr(sentenceBuffer[sentenceIndex], '*') + 1;
	if (pCs == nullptr)
		return;
	int16_t Cs = (NibbleValue(pCs[0]) << 4) | NibbleValue(pCs[1]);
	if (Cs < 0)
		return;

	uint8_t crc = 0;
	for (char *pC = sentenceBuffer[sentenceIndex] + 1; pC < (pCs - 1); pC++)
	{
		crc = crc ^ (*pC);
	}

	if (crc != Cs)
		return;

	uint32_t sId = ((uint8_t) sentenceBuffer[sentenceIndex][3]) << 16;
	sId |= ((uint8_t) sentenceBuffer[sentenceIndex][4]) << 8;
	sId |= ((uint8_t) sentenceBuffer[sentenceIndex][5]);

	char *pField = sentenceBuffer[sentenceIndex] + 7;
	switch (sId)
	{
	case 'RMB':
		DecodeRMBSentence(pField, navData);
		break;
	case 'RMC':
		DecodeRMCSentence(pField, navData);
		break;
	case 'GGA':
		DecodeGGASentence(pField, navData);
		break;
	case 'VTG':
		DecodeVTGSentence(pField, navData);
		break;
	}

	return;
}

void NmeaDecoder::DecodeRMBSentence(char *sentence, NavigationData *navData)
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
		navData->xte_nm.value = value;
		navData->xte_nm.valid = true;
		navData->xte_nm.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] == 'R')
		navData->xte_nm.value = -navData->xte_nm.value;
	for (int i = 0; i < 7; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) == 1)
	{
		navData->dtw_nm.value = value;
		navData->dtw_nm.valid = true;
		navData->dtw_nm.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		navData->btw_deg.value = value;
		navData->btw_deg.valid = true;
		navData->btw_deg.timeStamp = millis();
	}
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sscanf(sentence, "%f", &value) == 1)
	{
		navData->vmc_kt.value = value;
		navData->vmc_kt.valid = true;
		navData->vmc_kt.timeStamp = millis();
	}
}

void NmeaDecoder::DecodeRMCSentence(char *sentence, NavigationData *navData)
{
	if (sentence[0] != ',')
	{
		navData->time.hour = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		navData->time.minute = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		navData->time.valid = true;
		navData->time.timeStamp = millis();
	}
	for (int i = 0; i < 8; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sentence[0] != ',')
	{
		navData->date.day = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		navData->date.month = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		navData->date.year = (sentence[4] - '0') * 10 + (sentence[5] - '0');
		navData->date.valid = true;
		navData->date.timeStamp = millis();
	}
}

void NmeaDecoder::DecodeGGASentence(char *sentence, NavigationData *navData)
{
	float degs, mins;

	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;

	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		sscanf(sentence + 2, "%f,", &mins);
		navData->latitude_deg.value = degs + mins / 60.0f;
		if (sentence[8] == 'S')
			navData->latitude_deg.value = -navData->latitude_deg.value;
		navData->latitude_deg.valid = true;
		navData->latitude_deg.timeStamp = millis();
	}
	for (int i = 0; i < 2; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 100 + (sentence[1] - '0') * 10
				+ (sentence[2] - '0');
		sscanf(sentence + 2, "%f,", &mins);
		navData->longitude_deg.value = degs + mins / 60.0f;
		if (sentence[9] == 'W')
			navData->longitude_deg.value = -navData->longitude_deg.value;
		navData->longitude_deg.valid = true;
		navData->longitude_deg.timeStamp = millis();
	}
}

void NmeaDecoder::DecodeVTGSentence(char *sentence, NavigationData *navData)
{
	float value;

	// FIXME : We use true bearing for COG. Check coherency with what Micronet display are waiting
	if (sscanf(sentence, "%f", &value) == 1)
	{
		navData->cog_deg.value = value;
		navData->cog_deg.valid = true;
		navData->cog_deg.timeStamp = millis();
	}
	for (int i = 0; i < 4; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sscanf(sentence, "%f", &value) == 1)
	{
		navData->sog_kt.value = value;
		navData->sog_kt.valid = true;
		navData->sog_kt.timeStamp = millis();
	}
}

int16_t NmeaDecoder::NibbleValue(char c)
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
