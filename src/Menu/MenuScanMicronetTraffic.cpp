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

#include <Arduino.h>

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

/**
 * Print a byte as two hex digits with leading zero
 * 
 * @param data Byte to print (0x00-0xFF)
 */
void PrintByte(uint8_t data);

/**
 * Print a 32-bit value as 8 hex digits
 * 
 * @param data Value to print
 */
void PrintInt(uint32_t data);

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

    CONSOLE.println("Starting Micronet traffic scanning.");
    CONSOLE.println("Press ESC key at any time to stop scanning and come back to menu.");
    CONSOLE.println("");

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
                    CONSOLE.println("");
                    lastMasterRequest_us = message->endTime_us;
                    micronetCodec.GetNetworkMap(message, &networkMap);
                    CONSOLE.print("Network ID : 0x");
                    PrintInt(networkMap.networkId);
                    CONSOLE.println("");
                    //PrintNetworkMap(&networkMap);
                }
                PrintRawMessage(message, lastMasterRequest_us);
            }
            gRxMessageFifo.DeleteMessage();
        }

        while (CONSOLE.available() > 0)
        {
            if (CONSOLE.read() == 0x1b)
            {
                CONSOLE.println("ESC key pressed, stopping scan.");
                exitSniffLoop = true;
            }
        }
        yield();
    } while (!exitSniffLoop);
}

void PrintNetworkMap(MicronetCodec::NetworkMap *networkMap)
{
    CONSOLE.print("Network ID : 0x");
    PrintInt(networkMap->networkId);
    CONSOLE.println("");

    CONSOLE.print("Nb Devices : ");
    CONSOLE.println(networkMap->nbSyncSlots);
    CONSOLE.print("Master : ");
    CONSOLE.print(" : 0x");
    PrintInt(networkMap->masterDevice);
    CONSOLE.println("");

    for (uint32_t i = 0; i < networkMap->nbSyncSlots; i++)
    {
        CONSOLE.print("S");
        CONSOLE.print(i);
        CONSOLE.print(" : 0x");
        PrintInt(networkMap->syncSlot[i].deviceId);
        CONSOLE.print(" ");
        if (networkMap->syncSlot[i].start_us > 0)
        {
            CONSOLE.print(networkMap->syncSlot[i].payloadBytes);
            CONSOLE.print(" ");
            CONSOLE.print(networkMap->syncSlot[i].start_us - networkMap->firstSlot);
            CONSOLE.print(" ");
            CONSOLE.println(networkMap->syncSlot[i].length_us);
        }
        else
        {
            CONSOLE.println("-");
        }
    }

    CONSOLE.print("Async : ");
    CONSOLE.print(" ");
    CONSOLE.print(networkMap->asyncSlot.payloadBytes);
    CONSOLE.print(" ");
    CONSOLE.print(networkMap->asyncSlot.start_us - networkMap->firstSlot);
    CONSOLE.print(" ");
    CONSOLE.println(networkMap->asyncSlot.length_us);

    for (uint32_t i = 0; i < networkMap->nbAckSlots; i++)
    {
        CONSOLE.print("A");
        CONSOLE.print(i);
        CONSOLE.print(" : 0x");
        PrintInt(networkMap->ackSlot[i].deviceId);
        CONSOLE.print(" ");
        CONSOLE.print(networkMap->ackSlot[i].payloadBytes);
        CONSOLE.print(" ");
        CONSOLE.print(networkMap->ackSlot[i].start_us - networkMap->firstSlot);
        CONSOLE.print(" ");
        CONSOLE.println(networkMap->ackSlot[i].length_us);
    }
    CONSOLE.println("");
}

void PrintRawMessage(MicronetMessage_t *message, uint32_t lastMasterRequest_us)
{
    if (message->len < MICRONET_PAYLOAD_OFFSET)
    {
        CONSOLE.print("Invalid message (");
        CONSOLE.print((int)message->rssi);
        CONSOLE.print(", ");
        CONSOLE.print((int)(message->startTime_us - lastMasterRequest_us));
        CONSOLE.println(")");
    }

    for (int j = 0; j < 4; j++)
    {
        PrintByte(message->data[j]);
    }
    CONSOLE.print(" ");

    for (int j = 4; j < 8; j++)
    {
        PrintByte(message->data[j]);
    }
    CONSOLE.print(" ");

    for (int j = 8; j < 14; j++)
    {
        PrintByte(message->data[j]);
        CONSOLE.print(" ");
    }

    CONSOLE.print(" -- ");

    for (int j = 14; j < message->len; j++)
    {
        PrintByte(message->data[j]);
        CONSOLE.print(" ");
    }

    CONSOLE.print(" (");
    CONSOLE.print((int)message->rssi);
    CONSOLE.print(", ");
    CONSOLE.print((int)(message->startTime_us - lastMasterRequest_us));
    CONSOLE.print(")");

    CONSOLE.println();
}

void PrintByte(uint8_t data)
{
    if (data < 16)
    {
        CONSOLE.print("0");
    }
    CONSOLE.print(data, HEX);
}

void PrintInt(uint32_t data)
{
    PrintByte((data >> 24) & 0x0ff);
    PrintByte((data >> 16) & 0x0ff);
    PrintByte((data >> 8) & 0x0ff);
    PrintByte(data & 0x0ff);
}
