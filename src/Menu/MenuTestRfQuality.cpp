/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Test RF link quality between Micronet devices                 *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements an interactive RF quality tester that:           *
 * - Sends periodic ping messages to all network devices                   *
 * - Measures signal strength (RSSI) for each response                     *
 * - Reports link quality from devices to the master                       *
 * - Identifies device types and roles in the network                      *
 *                                                                         *
 * Note: This diagnostic tool monopolizes the async slot, which may        *
 * interfere with normal network operation. Not recommended during         *
 * navigation.                                                            *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021-2025 Ronan Demoment                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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

/* No file-local constants required */

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/* No local types required */

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/**
 * Print device type as human readable string
 * 
 * @param deviceType Device type code from Micronet message
 */
void PrintDeviceType(uint8_t deviceType);

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/* No file-local globals required */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuTestRfQuality
 *
 * Interactive RF link quality tester. This function:
 * - Monitors master requests to determine network timing
 * - Sends ping messages in the async slot
 * - Collects responses and measures signal strength
 * - Reports:
 *   * Device ID and type
 *   * Link quality to this converter (LNK)
 *   * Link quality to network master (NET)
 *   * Master device indicator [M]
 *
 * The function exits when user presses ESC.
 * Warning: Using this test during navigation is not recommended as it
 * interferes with normal network operation.
 */
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
                    micronetCodec.EncodePingMessage(&txMessage, 9, networkMap.networkId, gConfiguration.eeprom.deviceId);
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
                    PrintDeviceType(micronetCodec.GetDeviceType(message));
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

/**
 * Print device type as a human readable string
 * Maps Micronet device type codes to descriptive names
 * 
 * @param deviceType Device type code from Micronet message
 */
void PrintDeviceType(uint8_t deviceType)
{
    switch (deviceType) {
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
        case MICRONET_DEVICE_TYPE_COMPASS:
            CONSOLE.print("Compass");
            break;
        case MICRONET_DEVICE_TYPE_REMOTE_DISPLAY:
            CONSOLE.print("Remote Display");
            break;
        default:
            CONSOLE.print("Unknown");
            break;
    }
}
