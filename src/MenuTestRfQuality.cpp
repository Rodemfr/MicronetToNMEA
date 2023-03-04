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

#include <Arduino.h>

#include "BoardConfig.h"
#include "Configuration.h"
#include "Globals.h"
#include "Micronet.h"
#include "MicronetCodec.h"

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

// Test the quality of the radio link between MicronetToNMEA and all other devices. Also display the quality of the link between all devices and
// network's master device. This menu sends a ping command every network cycle and listens to responses from devices, collecting message RSSI and link
// quality byte reported by the transmitter. Be carreful, that this function monopolizes the asynchronous slot, forbidding other devices to work
// properly. Don't use it while navigating.
void MenuTestRfQuality()
{
    bool                      exitTestLoop = false;
    MicronetCodec::NetworkMap networkMap;
    TxSlotDesc_t              txSlot;
    MicronetMessage_t         txMessage;
    uint32_t                  receivedDid[MICRONET_MAX_DEVICES_PER_NETWORK];
    MicronetCodec             micronetCodec;
    char                      deviceString[128];

    CONSOLE.println("Starting RF signal quality test.");
    CONSOLE.println("Press ESC key at any time to stop testing and come back to menu.");
    CONSOLE.println("");

    gRxMessageFifo.ResetFifo();

    // Loop until ESC key is pressed
    do
    {
        MicronetMessage_t *message;

        // Read messages from RC FIFO
        if ((message = gRxMessageFifo.Peek()) != nullptr)
        {
            // Only consider messages with a correct CRC
            if (micronetCodec.VerifyHeaderCrc(message))
            {
                // Master Request ?
                if (message->data[MICRONET_MI_OFFSET] == MICRONET_MESSAGE_ID_MASTER_REQUEST)
                {
                    // Yes : Decode network map and encode a Ping message
                    CONSOLE.println("");
                    micronetCodec.GetNetworkMap(message, &networkMap);
                    txSlot = micronetCodec.GetAsyncTransmissionSlot(&networkMap);
                    micronetCodec.EncodePingMessage(&txMessage, 9, networkMap.networkId, gConfiguration.deviceId);
                    txMessage.action       = MICRONET_ACTION_RF_NO_ACTION;
                    txMessage.startTime_us = txSlot.start_us;
                    gRfReceiver.Transmit(&txMessage);
                    // Reset list of received devices
                    memset(receivedDid, 0, sizeof(receivedDid));
                }

                // Check if the transmitting device has already sent a message on this cycle
                bool alreadyReceived = false;
                for (int i = 0; i < MICRONET_MAX_DEVICES_PER_NETWORK; i++)
                {
                    uint32_t did = micronetCodec.GetDeviceId(message);
                    if (receivedDid[i] == did)
                    {
                        // Yes : mark the device as already processed on this cycle
                        alreadyReceived = true;
                        break;
                    }
                    else if (receivedDid[i] == 0)
                    {
                        receivedDid[i] = did;
                        break;
                    }
                }

                // Only print data if the device has not already been processed in this cycle
                if (!alreadyReceived)
                {
                    // Read network's strength (how strong the slave device is receiving Master's request)
                    uint8_t receptionStrength = micronetCodec.GetSignalStrength(message);
                    // If the device is the master : set the network strength to maximum value
                    if (networkMap.masterDevice == micronetCodec.GetDeviceId(message))
                    {
                        receptionStrength = 9.0f;
                    }

                    snprintf(deviceString, sizeof(deviceString), "%08lx LNK=%.1f NET=%1d ", micronetCodec.GetDeviceId(message),
                             micronetCodec.CalculateSignalFloatStrength(message), receptionStrength);
                    CONSOLE.print(deviceString);
                    // Device type
                    switch (micronetCodec.GetDeviceType(message))
                    {
                    case MICRONET_DEVICE_TYPE_HULL_TRANSMITTER:
                        CONSOLE.print("Hull");
                        break;
                    case MICRONET_DEVICE_TYPE_WIND_TRANSDUCER:
                        CONSOLE.print("Wind Transducer");
                        break;
                    case MICRONET_DEVICE_TYPE_NMEA_CONVERTER:
                        CONSOLE.print("NMEA Converter");
                        break;
                    case MICRONET_DEVICE_TYPE_MAST_ROTATION:
                        CONSOLE.print("Mast Rotation");
                        break;
                    case MICRONET_DEVICE_TYPE_MOB:
                        CONSOLE.print("MOB");
                        break;
                    case MICRONET_DEVICE_TYPE_SDPOD:
                        CONSOLE.print("SDPOD");
                        break;
                    case MICRONET_DEVICE_TYPE_DUAL_DISPLAY:
                        CONSOLE.print("Dual Display");
                        break;
                    case MICRONET_DEVICE_TYPE_ANALOG_WIND_DISPLAY:
                        CONSOLE.print("Wind Display");
                        break;
                    case MICRONET_DEVICE_TYPE_DUAL_MAXI_DISPLAY:
                        CONSOLE.print("Dual Maxi Display");
                        break;
                    case MICRONET_DEVICE_TYPE_REMOTE_DISPLAY:
                        CONSOLE.print("Remote Display");
                        break;
                    default:
                        CONSOLE.print("Unknown");
                        break;
                    }
                    if (networkMap.masterDevice == micronetCodec.GetDeviceId(message))
                    {
                        CONSOLE.print(" [M]");
                    }
                    CONSOLE.println("");
                }
            }
            // Delete processed message
            gRxMessageFifo.DeleteMessage();
        }

        // Check if ESC key has been pressed
        while (CONSOLE.available() > 0)
        {
            if (CONSOLE.read() == 0x1b)
            {
                CONSOLE.println("ESC key pressed, stopping scan.");
                exitTestLoop = true;
            }
        }

        // Let Arduino's processing loop handle what it has to handle...
        yield();
    } while (!exitTestLoop);
}
