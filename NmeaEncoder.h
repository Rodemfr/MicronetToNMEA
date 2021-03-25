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

#ifndef NMEAENCODER_H_
#define NMEAENCODER_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include <string>
#include "MicronetCodec.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

using namespace std;

typedef struct {
	uint32_t vwr;
	uint32_t vwt;
	uint32_t dpt;
	uint32_t mtw;
	uint32_t vlw;
	uint32_t vhw;
} SentencesTimeStamps_t;

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class NmeaEncoder
{
public:
	NmeaEncoder();
	virtual ~NmeaEncoder();

	bool EncodeMWV_R(MicronetData_t *micronetData, char *sentence);
	bool EncodeMWV_T(MicronetData_t *micronetData, char *sentence);
	bool EncodeDPT(MicronetData_t *micronetData, char *sentence);
	bool EncodeMTW(MicronetData_t *micronetData, char *sentence);
	bool EncodeVLW(MicronetData_t *micronetData, char *sentence);
	bool EncodeVHW(MicronetData_t *micronetData, char *sentence);

private:
	SentencesTimeStamps_t timeStamps;
	uint8_t AddNmeaChecksum(char *sentence);
};

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* NMEAENCODER_H_ */
