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

typedef enum {
		LINK_NMEA_EXT,
		LINK_NMEA_GNSS,
		LINK_MICRONET,
		LINK_COMPASS
} LinkId_t;

typedef enum {
	NMEA_ID_UNKNOWN,
	NMEA_ID_RMB,
	NMEA_ID_RMC,
	NMEA_ID_GGA,
	NMEA_ID_VTG
} NmeaId_t;

#define NAV_SOURCE_LINK     LINK_NMEA_EXT
#define GNSS_SOURCE_LINK    LINK_NMEA_GNSS
#define WIND_SOURCE_LINK    LINK_MICRONET
#define DEPTH_SOURCE_LINK   LINK_MICRONET
#define SPEED_SOURCE_LINK   LINK_MICRONET
#define COMPASS_SOURCE_LINK LINK_COMPASS

class DataBridge
{
public:
	DataBridge(NavigationData *navData);
	virtual ~DataBridge();

	void PushNmeaChar(char c, LinkId_t sourceLink);
	void UpdateCompassData(float heading_deg);
	void UpdateMicronetData(NavigationData *navData);

private:
	NavigationData *micronetData;
	char nmeaExtBuffer[NMEA_SENTENCE_MAX_LENGTH];
	char nmeaGnssBuffer[NMEA_SENTENCE_MAX_LENGTH];
	int nmeaExtWriteIndex;
	int nmeaGnssWriteIndex;

	bool IsSentenceValid(char *nmeaBuffer);
	NmeaId_t SentenceId(char *nmeaBuffer);
	void DecodeRMBSentence(char *sentence);
	void DecodeRMCSentence(char *sentence);
	void DecodeGGASentence(char *sentence);
	void DecodeVTGSentence(char *sentence);
	int16_t NibbleValue(char c);
};

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* NMEADECODER_H_ */
