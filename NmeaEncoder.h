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

#include "MicronetDecoder.h"

#include <string>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

using namespace std;

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class NmeaEncoder
{
public:
	NmeaEncoder();
	virtual ~NmeaEncoder();

	bool EncodeINVWR(MicronetData_t *micronetData, char *sentence);
	bool EncodeINVWT(MicronetData_t *micronetData, char *sentence);

private:
	uint32_t INVWRTimeStamp;
	uint32_t INVWTTimeStamp;

	uint8_t NmeaChecksum(char *sentence);
};

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* NMEAENCODER_H_ */
