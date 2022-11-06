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

#include "MicronetSlaveDevice.h"
#include "Globals.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define NETWORK_TIMEOUT_MS 3000

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

MicronetSlaveDevice::MicronetSlaveDevice() :
		deviceId(0), networkId(0), dataFields(0), latestSignalStrength(0), firstSlot(0), networkStatus(NETWORK_STATUS_NOT_FOUND), lastNetworkMessage_us(0)
{
	memset(&networkMap, 0, sizeof(networkMap));
}

MicronetSlaveDevice::~MicronetSlaveDevice()
{
}

void MicronetSlaveDevice::SetDeviceId(uint32_t deviceId)
{
	this->deviceId = deviceId;
}

void MicronetSlaveDevice::SetNetworkId(uint32_t networkId)
{
	this->networkId = networkId;
}

void MicronetSlaveDevice::SetDataFields(uint32_t dataFields)
{
	this->dataFields = dataFields;
}

void MicronetSlaveDevice::ProcessMessage(MicronetMessage_t *message, MicronetMessageFifo *messageFifo)
{
	TxSlotDesc_t txSlot;
	MicronetMessage_t txMessage;

	if ((networkStatus == NETWORK_STATUS_FOUND) && ((micros() - lastNetworkMessage_us) > NETWORK_TIMEOUT_MS * 1000))
	{
		networkStatus = NETWORK_STATUS_NOT_FOUND;
		txMessage.action = MICRONET_ACTION_RF_ACTIVE_POWER;
		txMessage.startTime_us = micros() + 100000;
		messageFifo->Push(txMessage);
	}

	if ((micronetCodec.GetNetworkId(message) == networkId) && (micronetCodec.VerifyHeaderCrc(message)))
	{
		if (micronetCodec.GetMessageId(message) == MICRONET_MESSAGE_ID_MASTER_REQUEST)
		{
			networkStatus = NETWORK_STATUS_FOUND;
			lastNetworkMessage_us = message->startTime_us;
			firstSlot = message->endTime_us;
			micronetCodec.GetNetworkMap(message, &networkMap);

			txMessage.action = MICRONET_ACTION_RF_LOW_POWER;
			txMessage.startTime_us = micronetCodec.GetEndOfNetwork(&networkMap);
			messageFifo->Push(txMessage);

			txMessage.action = MICRONET_ACTION_RF_ACTIVE_POWER;
			txMessage.startTime_us = micronetCodec.GetNextStartOfNetwork(&networkMap) - 500;
			messageFifo->Push(txMessage);

			latestSignalStrength = micronetCodec.CalculateSignalStrength(message);
			txSlot = micronetCodec.GetSyncTransmissionSlot(&networkMap, deviceId);
			if (txSlot.start_us != 0)
			{
				// TODO : move NavigationData to MicronetCodec
				uint32_t payloadLength = micronetCodec.EncodeDataMessage(&txMessage, latestSignalStrength, networkId, deviceId, &gNavData,
						dataFields);
				if (txSlot.payloadBytes < payloadLength)
				{
					txSlot = micronetCodec.GetAsyncTransmissionSlot(&networkMap);
					micronetCodec.EncodeSlotUpdateMessage(&txMessage, latestSignalStrength, networkId, deviceId, payloadLength);
				}
			}
			else
			{
				txSlot = micronetCodec.GetAsyncTransmissionSlot(&networkMap);
				micronetCodec.EncodeSlotRequestMessage(&txMessage, latestSignalStrength, networkId, deviceId, micronetCodec.GetDataMessageLength(dataFields));
			}

			txMessage.action = MICRONET_ACTION_RF_NO_ACTION;
			txMessage.startTime_us = txSlot.start_us;
			messageFifo->Push(txMessage);
		} else {
			// TODO : look for the best place where to put Nav data
			if (gMicronetCodec.DecodeMessage(message, &gNavData))
			{
				txSlot = micronetCodec.GetAckTransmissionSlot(&networkMap, deviceId);
				micronetCodec.EncodeAckParamMessage(&txMessage, latestSignalStrength, networkId, deviceId);
				txMessage.action = MICRONET_ACTION_RF_NO_ACTION;
				txMessage.startTime_us = txSlot.start_us;
				messageFifo->Push(txMessage);
			}
		}
	}
}
