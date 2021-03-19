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

#include "GnssDecoder.h"

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

GnssDecoder::GnssDecoder()
{
	writeIndex = 0;
	sentenceWriteIndex = 0;
	serialBuffer[0] = 0;
}

GnssDecoder::~GnssDecoder()
{
}

void GnssDecoder::PushChar(char c)
{
	if (serialBuffer[0] != '$')
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

int GnssDecoder::GetNbSentences()
{
	return sentenceWriteIndex;
}

const char *GnssDecoder::GetSentence(int i)
{
	return sentenceBuffer[i];
}

void GnssDecoder::resetSentences()
{
	sentenceWriteIndex = 0;
}
