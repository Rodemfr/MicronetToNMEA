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
#include "BoardConfig.h"
#include "Globals.h"

/***************************************************************************/
/*                              Constants                                  */
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

MicronetSlaveDevice::MicronetSlaveDevice(MicronetCodec *micronetCodec) : deviceId(0), networkId(0), dataFields(0), latestSignalStrength(0)
{
    memset(&networkMap, 0, sizeof(networkMap));
    this->micronetCodec = micronetCodec;
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
    TxSlotDesc_t      txSlot;
    MicronetMessage_t txMessage;

    if ((micronetCodec->GetNetworkId(message) == networkId) && (micronetCodec->VerifyHeaderCrc(message)))
    {
        if (micronetCodec->GetMessageId(message) == MICRONET_MESSAGE_ID_MASTER_REQUEST)
        {
            micronetCodec->GetNetworkMap(message, &networkMap);

            // We schedule the low power mode of CC1101 just at the end of the network cycle
            txMessage.action       = MICRONET_ACTION_RF_LOW_POWER;
            txMessage.startTime_us = micronetCodec->GetEndOfNetwork(&networkMap);
            txMessage.len          = 0;
            messageFifo->Push(txMessage);

            // We schedule exit of CC1101's low power mode 1ms before actual start of the next network cycle.
            // It will let time for the PLL calibration loop to complete.
            txMessage.action       = MICRONET_ACTION_RF_ACTIVE_POWER;
            txMessage.startTime_us = micronetCodec->GetNextStartOfNetwork(&networkMap) - 1000;
            txMessage.len          = 0;
            messageFifo->Push(txMessage);

            latestSignalStrength = micronetCodec->CalculateSignalStrength(message);

            for (int i = 0; i < NUMBER_OF_VIRTUAL_SLAVES; i++)
            {
                txSlot = micronetCodec->GetSyncTransmissionSlot(&networkMap, deviceId + i);
                if (txSlot.start_us != 0)
                {
                    uint32_t payloadLength =
                        micronetCodec->EncodeDataMessage(&txMessage, latestSignalStrength, networkId, deviceId + i, splitDataFields[i]);
                    if (txSlot.payloadBytes < payloadLength)
                    {
                        txSlot = micronetCodec->GetAsyncTransmissionSlot(&networkMap);
                        micronetCodec->EncodeSlotUpdateMessage(&txMessage, latestSignalStrength, networkId, deviceId + i, payloadLength);
                    }
                }
                else
                {
                    txSlot = micronetCodec->GetAsyncTransmissionSlot(&networkMap);
                    micronetCodec->EncodeSlotRequestMessage(&txMessage, latestSignalStrength, networkId, deviceId + i,
                                                            micronetCodec->GetDataMessageLength(splitDataFields[i]));
                }

                txMessage.action       = MICRONET_ACTION_RF_NO_ACTION;
                txMessage.startTime_us = txSlot.start_us;
                messageFifo->Push(txMessage);
            }
        }
        else
        {
            if (micronetCodec->DecodeMessage(message))
            {
                for (int i = 0; i < NUMBER_OF_VIRTUAL_SLAVES; i++)
                {
                    txSlot = micronetCodec->GetAckTransmissionSlot(&networkMap, deviceId + i);
                    micronetCodec->EncodeAckParamMessage(&txMessage, latestSignalStrength, networkId, deviceId + i);
                    txMessage.action       = MICRONET_ACTION_RF_NO_ACTION;
                    txMessage.startTime_us = txSlot.start_us;
                    messageFifo->Push(txMessage);
                }
            }
        }
    }
}

// Distribute requested data fields to the virtual slave devices
// This distribution is made to balance the size of the data message of each slave
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
}

// Returns the index of the virtual slave with the shortest data message
uint8_t MicronetSlaveDevice::GetShortestSlave()
{
    uint8_t  minSlaveSize  = 255;
    uint32_t minSlaveIndex = 0;

    for (int i = 0; i < NUMBER_OF_VIRTUAL_SLAVES; i++)
    {
        uint8_t size = micronetCodec->GetDataMessageLength(splitDataFields[i]);
        if (size < minSlaveSize)
        {
            minSlaveSize  = size;
            minSlaveIndex = i;
        }
    }

    return minSlaveIndex;
}
