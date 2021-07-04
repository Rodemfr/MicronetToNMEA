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

#ifndef NMEADECODER_H_
#define NMEADECODER_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include <stdint.h>
#include "NavigationData.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define NMEA_SENTENCE_MAX_LENGTH   96
#define NMEA_SENTENCE_HISTORY_SIZE 24

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

class NmeaDecoder
{
public:
	NmeaDecoder();
	virtual ~NmeaDecoder();

	void PushChar(char c, NavigationData *navData);
	int GetNbSentences();
	const char *GetSentence(int i);
	void resetSentences();

private:
	uint8_t serialBuffer[NMEA_SENTENCE_MAX_LENGTH];
	int writeIndex;
	char sentenceBuffer[NMEA_SENTENCE_HISTORY_SIZE][NMEA_SENTENCE_MAX_LENGTH];
	int sentenceWriteIndex;

	void DecodeSentence(int sentenceIndex, NavigationData *navData);
	void DecodeRMBSentence(char *sentence, NavigationData *navData);
	void DecodeRMCSentence(char *sentence, NavigationData *navData);
	void DecodeGGASentence(char *sentence, NavigationData *navData);
	void DecodeVTGSentence(char *sentence, NavigationData *navData);
	int16_t NibbleValue(char c);
};

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* NMEADECODER_H_ */
