/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Monitor and analyze Micronet radio traffic                    *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements a diagnostic tool that scans, decodes and        *
 * displays Micronet network traffic. It provides real-time monitoring of: *
 * - Raw message contents                                                  *
 * - Message timing relative to master requests                           *
 * - Signal strength (RSSI)                                               *
 * - Network topology (master, slaves, slots)                             *
 *                                                                         *
 * The implementation is useful for:                                       *
 * - Network debugging and troubleshooting                                *
 * - Understanding network timing and topology                            *
 * - Verifying RF reception quality                                       *
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

#include "BoardConfig.h"
#include "Configuration.h"
#include "Globals.h"
#include "Micronet.h"
#include "MicronetCodec.h"
#include "MicronetMessageFifo.h"

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
 * Print detailed network topology information
 *
 * @param networkMap Pointer to NetworkMap containing topology data
 */
void PrintNetworkMap(MicronetCodec::NetworkMap *networkMap);

/**
 * Print raw message content with timing and signal strength
 *
 * @param message Pointer to received message
 * @param lastMasterRequest_us Timestamp of last master request for relative timing
 */
void PrintRawMessage(MicronetMessage_t *message, uint32_t lastMasterRequest_us);

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuScanMicronetTraffic
 *
 * Main scanning loop that:
 * - Initializes message FIFO and reception
 * - Processes received messages
 * - Displays message content, timing and signal strength
 * - Updates network map on master requests
 * - Exits when user presses ESC
 *
 * The function performs blocking console I/O but yields regularly
 * to allow background tasks to run.
 */
void MenuScanMicronetTraffic()
{
    bool                      exitSniffLoop        = false;
    uint32_t                  lastMasterRequest_us = 0;
    MicronetCodec::NetworkMap networkMap;
    MicronetCodec             micronetCodec;

    CONSOLE.printf("Starting Micronet traffic scanning.\r\n");
    CONSOLE.printf("Press ESC key at any time to stop scanning and come back to menu.\r\n");
    CONSOLE.transmit();

    gRfReceiver.RestartReception();

    gRxMessageFifo.ResetFifo();

    MicronetMessage_t *message;
    do
    {
        if ((message = gRxMessageFifo.Peek()) != nullptr)
        {
            if (micronetCodec.VerifyHeaderCrc(message))
            {
                if (message->data[MICRONET_MI_OFFSET] == MICRONET_MESSAGE_ID_MASTER_REQUEST)
                {
                    CONSOLE.printf("\r\n");
                    lastMasterRequest_us = message->endTime_us;
                    micronetCodec.GetNetworkMap(message, &networkMap);
                    CONSOLE.printf("Network ID : 0x%x\r\n", networkMap.networkId);
                    // PrintNetworkMap(&networkMap);
                }
                PrintRawMessage(message, lastMasterRequest_us);
                CONSOLE.transmit();
            }
            gRxMessageFifo.DeleteMessage();
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Yield to allow background tasks to run

        while (CONSOLE.available() > 0)
        {
            if (CONSOLE.read() == 0x1b)
            {
                CONSOLE.printf("ESC key pressed, stopping scan.\r\n");
                CONSOLE.transmit();

                exitSniffLoop = true;
            }
        }

    } while (!exitSniffLoop);
}

void PrintNetworkMap(MicronetCodec::NetworkMap *networkMap)
{
    CONSOLE.printf("Network ID : 0x%x\r\n", networkMap->networkId);
    CONSOLE.printf("Nb Devices : %d\r\n", networkMap->nbSyncSlots);
    CONSOLE.printf("Master : 0x%x\r\n", networkMap->masterDevice);

    for (uint32_t i = 0; i < networkMap->nbSyncSlots; i++)
    {
        CONSOLE.printf("S%d : 0x%x\r\n", i, networkMap->syncSlot[i].deviceId);
        if (networkMap->syncSlot[i].start_us > 0)
        {
            CONSOLE.printf("Payload Bytes: %d\r\n", networkMap->syncSlot[i].payloadBytes);
            CONSOLE.printf("%u ", networkMap->syncSlot[i].start_us - networkMap->firstSlot);
            CONSOLE.printf("%u\r\n", networkMap->syncSlot[i].length_us);
        }
        else
        {
            CONSOLE.printf("-\r\n");
        }
    }

    CONSOLE.printf("Async : ");
    CONSOLE.printf(" ");
    CONSOLE.printf("%d ", networkMap->asyncSlot.payloadBytes);
    CONSOLE.printf("%u ", networkMap->asyncSlot.start_us - networkMap->firstSlot);
    CONSOLE.printf("%u\r\n", networkMap->asyncSlot.length_us);

    for (uint32_t i = 0; i < networkMap->nbAckSlots; i++)
    {
        CONSOLE.printf("A%d : 0x%x\r\n", i, networkMap->ackSlot[i].deviceId);
        CONSOLE.printf(" ");
        CONSOLE.printf("%d ", networkMap->ackSlot[i].payloadBytes);
        CONSOLE.printf(" ");
        CONSOLE.printf("%u ", networkMap->ackSlot[i].start_us - networkMap->firstSlot);
        CONSOLE.printf("%u\r\n", networkMap->ackSlot[i].length_us);
    }
    CONSOLE.printf("\r\n");
}

void PrintRawMessage(MicronetMessage_t *message, uint32_t lastMasterRequest_us)
{
    if (message->len < MICRONET_PAYLOAD_OFFSET)
    {
        CONSOLE.printf("Invalid message (");
        CONSOLE.printf("%d ", (int)message->rssi);
        CONSOLE.printf(", ");
        CONSOLE.printf("%d)\r\n", (int)(message->startTime_us - lastMasterRequest_us));
    }

    for (int j = 0; j < 4; j++)
    {
        CONSOLE.printf("%02x", message->data[j]);
    }
    CONSOLE.printf(" ");

    for (int j = 4; j < 8; j++)
    {
        CONSOLE.printf("%02x", message->data[j]);
    }
    CONSOLE.printf(" ");

    for (int j = 8; j < 14; j++)
    {
        CONSOLE.printf("%02x ", message->data[j]);
    }

    CONSOLE.printf(" -- ");

    for (int j = 14; j < message->len; j++)
    {
        CONSOLE.printf("%02x ", message->data[j]);
    }

    CONSOLE.printf(" (");
    CONSOLE.printf("%d ", (int)message->rssi);
    CONSOLE.printf(", ");
    CONSOLE.printf("%d)\r\n", (int)(message->startTime_us - lastMasterRequest_us));
}
