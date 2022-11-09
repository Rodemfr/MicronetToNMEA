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
#include "BoardConfig.h"

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
		deviceId(0), networkId(0), dataFields(0), latestSignalStrength(0), firstSlot(0), networkStatus(
				NETWORK_STATUS_NOT_FOUND), lastNetworkMessage_us(0)
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

	SplitDataFields();
}

void MicronetSlaveDevice::AddDataFields(uint32_t dataFields)
{
	this->dataFields |= dataFields;

	SplitDataFields();
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
			// If we reach this point, it means we have found our network
			networkStatus = NETWORK_STATUS_FOUND;
			lastNetworkMessage_us = message->startTime_us;
			firstSlot = message->endTime_us;
			micronetCodec.GetNetworkMap(message, &networkMap);

			// We schedule the low power mode of CC1101 just at the end of the network cycle
			txMessage.action = MICRONET_ACTION_RF_LOW_POWER;
			txMessage.startTime_us = micronetCodec.GetEndOfNetwork(&networkMap);
			messageFifo->Push(txMessage);

			// We schedule exit of CC1101's low power mode 1ms before actual start of the next network cycle.
			// It will let time for the PLL calibration loop to complete.
			txMessage.action = MICRONET_ACTION_RF_ACTIVE_POWER;
			txMessage.startTime_us = micronetCodec.GetNextStartOfNetwork(&networkMap) - 1000;
			messageFifo->Push(txMessage);

			for (int i = 0; i < NUMBER_OF_VIRTUAL_SLAVES; i++)
			{
				latestSignalStrength = micronetCodec.CalculateSignalStrength(message);
				txSlot = micronetCodec.GetSyncTransmissionSlot(&networkMap, deviceId + i);
				if (txSlot.start_us != 0)
				{
					// TODO : move NavigationData to MicronetCodec
					uint32_t payloadLength = micronetCodec.EncodeDataMessage(&txMessage, latestSignalStrength,
							networkId, deviceId + i, &gNavData, splitDataFields[i]);
					if (txSlot.payloadBytes < payloadLength)
					{
						txSlot = micronetCodec.GetAsyncTransmissionSlot(&networkMap);
						micronetCodec.EncodeSlotUpdateMessage(&txMessage, latestSignalStrength, networkId, deviceId + i,
								payloadLength);
					}
				}
				else
				{
					txSlot = micronetCodec.GetAsyncTransmissionSlot(&networkMap);
					micronetCodec.EncodeSlotRequestMessage(&txMessage, latestSignalStrength, networkId, deviceId + i,
							micronetCodec.GetDataMessageLength(splitDataFields[i]));
				}

				txMessage.action = MICRONET_ACTION_RF_NO_ACTION;
				txMessage.startTime_us = txSlot.start_us;
				messageFifo->Push(txMessage);
			}
		}
		else
		{
			// TODO : look for the best place where to put Nav data
			if (gMicronetCodec.DecodeMessage(message, &gNavData))
			{
				for (int i = 0; i < NUMBER_OF_VIRTUAL_SLAVES; i++)
				{
					txSlot = micronetCodec.GetAckTransmissionSlot(&networkMap, deviceId + i);
					micronetCodec.EncodeAckParamMessage(&txMessage, latestSignalStrength, networkId, deviceId + i);
					txMessage.action = MICRONET_ACTION_RF_NO_ACTION;
					txMessage.startTime_us = txSlot.start_us;
					messageFifo->Push(txMessage);
				}
			}
		}
	}
}

// Split the request data fields to the virtual slave devices
void MicronetSlaveDevice::SplitDataFields()
{
	// Clear current split data fields
	for (int i = 0; i < NUMBER_OF_VIRTUAL_SLAVES; i++)
	{
		splitDataFields[i] = 0;
	}

	// Spread fields onto the virtual slave devices
	for (int i = 0; i < 32; i++)
	{
		if ((dataFields >> i) & 0x1)
		{
			splitDataFields[GetShortestSlave()] |= (1 << i);
		}
	}

	for (int i = 0; i < NUMBER_OF_VIRTUAL_SLAVES; i++)
	{
		CONSOLE.print("Slave ");
		CONSOLE.print(i);
		CONSOLE.print(" = ");
		CONSOLE.print(splitDataFields[i], HEX);
		CONSOLE.print(" (");
		CONSOLE.print(micronetCodec.GetDataMessageLength(splitDataFields[i]));
		CONSOLE.println(")");
	}
}

uint8_t MicronetSlaveDevice::GetShortestSlave()
{
	uint8_t minSlaveSize = 1000;
	uint32_t minSlaveIndex = 0;

	for (int i = 0; i < NUMBER_OF_VIRTUAL_SLAVES; i++)
	{
		uint8_t size = micronetCodec.GetDataMessageLength(splitDataFields[i]);
		if (size < minSlaveSize)
		{
			minSlaveSize = size;
			minSlaveIndex = i;
		}
	}

	return minSlaveIndex;
}
