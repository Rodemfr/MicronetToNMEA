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
	memset(&nmeaData, 0, sizeof(nmeaData));
}

NmeaDecoder::~NmeaDecoder()
{
}

void NmeaDecoder::PushChar(char c)
{
	if ((serialBuffer[0] != '$') || (c == '$'))
	{
		serialBuffer[0] = c;
		writeIndex = 1;
		return;
	}

	if (c == 13)
	{
		if ((writeIndex >= 10) && (sentenceWriteIndex < NMEA_SENTENCE_HISTORY_SIZE))
		{
			memcpy(sentenceBuffer[sentenceWriteIndex], serialBuffer, writeIndex);
			sentenceBuffer[sentenceWriteIndex][writeIndex] = 0;
			DecodeSentence(sentenceWriteIndex);
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

NmeaData_t* NmeaDecoder::GetCurrentData()
{
	return &nmeaData;
}

void NmeaDecoder::DecodeSentence(int sentenceIndex)
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
	case 'RMC':
		DecodeRMCSentence(pField);
		break;
	case 'GGA':
		DecodeGGASentence(pField);
		break;
	case 'VTG':
		DecodeVTGSentence(pField);
		break;
	}

	return;
}

void NmeaDecoder::DecodeRMCSentence(char *sentence)
{
	if (sentence[0] != ',')
	{
		nmeaData.hour = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		nmeaData.minute = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		nmeaData.timeUpdated = true;
	}
	for (int i = 0; i < 8; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sentence[0] != ',')
	{
		nmeaData.day = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		nmeaData.month = (sentence[2] - '0') * 10 + (sentence[3] - '0');
		nmeaData.year = (sentence[4] - '0') * 10 + (sentence[5] - '0');
		nmeaData.dateUpdated = true;
	}
}

void NmeaDecoder::DecodeGGASentence(char *sentence)
{
	float degs, mins;
	if ((sentence = strchr(sentence, ',')) == nullptr)
		return;
	sentence++;
	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 10 + (sentence[1] - '0');
		mins = (sentence[2] - '0') * 10 + (sentence[3] - '0')
				+ ((sentence[4] - '0') / 10.0f + (sentence[5] - '0') / 100.0f + (sentence[6] - '0') / 1000.0f);
		nmeaData.latitude = degs + mins / 60.0f;
		if (sentence[8] == 'S')
			nmeaData.latitude = -nmeaData.latitude;
		nmeaData.positionUpdated = true;
	}
	for (int i = 0; i < 2; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if (sentence[0] != ',')
	{
		degs = (sentence[0] - '0') * 100 + (sentence[1] - '0') * 10 + (sentence[2] - '0');
		mins = (sentence[3] - '0') * 10 + (sentence[4] - '0')
				+ ((sentence[5] - '0') / 10.0f + (sentence[6] - '0') / 100.0f + (sentence[7] - '0') / 1000.0f);
		nmeaData.longitude = degs + mins / 60.0f;
		if (sentence[9] == 'W')
			nmeaData.longitude = -nmeaData.longitude;
		nmeaData.positionUpdated = true;
	}
}

void NmeaDecoder::DecodeVTGSentence(char *sentence)
{
	float value;
	for (int i = 0; i < 2; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if ((value = sscanf(sentence, "%f", &value)) == 1)
	{
		nmeaData.cog = value;
		nmeaData.sogCogUpdated = true;
	}
	for (int i = 0; i < 2; i++)
	{
		if ((sentence = strchr(sentence, ',')) == nullptr)
			return;
		sentence++;
	}
	if ((value = sscanf(sentence, "%f", &value)) == 1)
	{
		nmeaData.sog = value;
		nmeaData.sogCogUpdated = true;
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
