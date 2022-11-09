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

#ifndef MICRONETSLAVEDEVICE_H_
#define MICRONETSLAVEDEVICE_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "Micronet.h"
#include "MicronetCodec.h"
#include "MicronetMessageFifo.h"
#include <Arduino.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define NUMBER_OF_VIRTUAL_SLAVES 3

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

typedef enum
{
	NETWORK_STATUS_NOT_FOUND = 0,
	NETWORK_STATUS_FOUND
} NetworkStatus_t;

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class MicronetSlaveDevice
{
public:
	MicronetSlaveDevice();
	virtual ~MicronetSlaveDevice();

	void SetDeviceId(uint32_t deviceId);
	void SetNetworkId(uint32_t networkId);
	void SetDataFields(uint32_t dataMask);
	void AddDataFields(uint32_t dataMask);
	void ProcessMessage(MicronetMessage_t *message, MicronetMessageFifo *messageFifo);

private:
	MicronetCodec micronetCodec;
	MicronetCodec::NetworkMap networkMap;
	uint32_t deviceId;
	uint32_t networkId;
	uint32_t dataFields;
	uint32_t splitDataFields[NUMBER_OF_VIRTUAL_SLAVES];
	uint8_t latestSignalStrength;
	uint32_t firstSlot;
	NetworkStatus_t networkStatus;
	uint32_t lastNetworkMessage_us;

	void SplitDataFields();
	uint8_t GetShortestSlave();
};

#endif /* MICRONETSLAVEDEVICE_H_ */
