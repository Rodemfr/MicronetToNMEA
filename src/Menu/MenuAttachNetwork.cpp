/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Scan for nearby Micronet networks and allow the user to       *
 *           attach the converter to the closest one.                      *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements the "Attach to network" menu action. It scans    *
 * the air for Micronet packets for a short period, collects distinct      *
 * network identifiers (NID) ordered by RSSI, displays the results to the  *
 * user and optionally writes the selected NID into persistent EEPROM.     *
 *                                                                         *
 * Responsibilities:
 *  - Listen for incoming Micronet messages during a short scan window
 *  - Verify message CRC before using the message for network discovery
 *  - Maintain a small sorted list of discovered networks by RSSI
 *  - Present scan results to the user and prompt for confirmation
 *  - Persist the chosen network id into configuration EEPROM
 *
 * The implementation is intentionally simple and blocking (console-driven).
 * It is suitable for an interactive configuration menu where the user
 * expects immediate feedback.
 *
 ***************************************************************************
 *   Copyright (C) 2021 by Ronan Demoment                                  *
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

extern "C"
{
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
}

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* Maximum number of distinct networks to keep during the scan window. */
#define MAX_SCANNED_NETWORKS 8

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/* (No file-local types required) */

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/**
 * MenuAttachNetwork
 *
 * Scan the radio for Micronet networks during a fixed time window,
 * collect unique network identifiers (NID) ordered by signal strength (RSSI),
 * display the list to the user and offer to attach the converter to the
 * strongest/closest network.
 *
 * This function is synchronous and returns after the scan and optional
 * user confirmation. It updates gConfiguration.eeprom.networkId and
 * persists the change when the user confirms.
 */
void MenuAttachNetwork(void);

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/* (No file-local globals required) */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

void MenuAttachNetwork()
{
    MicronetMessage_t *message;
    uint32_t           nidArray[MAX_SCANNED_NETWORKS];
    int16_t            rssiArray[MAX_SCANNED_NETWORKS];
    MicronetCodec      micronetCodec;

    memset(nidArray, 0, sizeof(nidArray));
    memset(rssiArray, 0, sizeof(rssiArray));

    // Scan micronet networks for 5s
    CONSOLE.printf("Scanning Micronet networks for 5 seconds ... ");
    CONSOLE.transmit();
    gRxMessageFifo.ResetFifo();
    unsigned long startTime = static_cast<uint32_t>(esp_timer_get_time()) / 1000; // Convert to milliseconds
    do
    {
        // Have we received a message ?
        if ((message = gRxMessageFifo.Peek()) != nullptr)
        {
            // Only consider messages with a valid CRC
            if (micronetCodec.VerifyHeaderCrc(message))
            {
                uint32_t nid  = micronetCodec.GetNetworkId(message);
                int16_t  rssi = message->rssi;
                // Store the network in the array by order of reception power
                for (int i = 0; i < MAX_SCANNED_NETWORKS; i++)
                {
                    if (nidArray[i] == 0)
                    {
                        // New network
                        nidArray[i]  = nid;
                        rssiArray[i] = rssi;
                        break;
                    }
                    else if (nidArray[i] == nid)
                    {
                        // Already scanned network : update RSSI if stronger
                        if (rssi > rssiArray[i])
                        {
                            rssiArray[i] = rssi;
                        }
                        break;
                    }
                    else
                    {
                        // New network to be inserted in the list : shift the list down
                        if (rssi > rssiArray[i])
                        {
                            for (int j = (MAX_SCANNED_NETWORKS - 1); j > i; j--)
                            {
                                nidArray[j]  = nidArray[j - 1];
                                rssiArray[j] = rssiArray[j - 1];
                            }
                            nidArray[i]  = nid;
                            rssiArray[i] = rssi;
                            break;
                        }
                    }
                }
            }
            gRxMessageFifo.DeleteMessage();
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Yield to other tasks to avoid starving them
    } while ((static_cast<uint32_t>(esp_timer_get_time()) / 1000 - startTime) < 5000);

    CONSOLE.printf("done\r\n");
    CONSOLE.transmit();

    // Print results
    uint32_t closestNetwork = 0;
    int16_t  closestRssi    = INT16_MIN;
    if (nidArray[0] != 0)
    {
        CONSOLE.printf("List of scanned networks :\r\n");
        for (int i = 0; i < MAX_SCANNED_NETWORKS; i++)
        {
            if (nidArray[i] != 0)
            {
                if (rssiArray[i] > closestRssi)
                {
                    closestRssi    = rssiArray[i];
                    closestNetwork = nidArray[i];
                }
                CONSOLE.printf("Network %d - %04X (", i, nidArray[i]);
                if (rssiArray[i] < 70)
                    CONSOLE.printf("very strong\r\n");
                else if (rssiArray[i] < 80)
                    CONSOLE.printf("strong\r\n");
                else if (rssiArray[i] < 90)
                    CONSOLE.printf("normal");
                else
                    CONSOLE.printf("low");
            }
        }
    }
    else
    {
        CONSOLE.printf("/!\\ No Micronet network found /!\\");
        CONSOLE.printf("Check that your Micronet network is powered on.");
        return;
    }
    CONSOLE.printf("Closest network : ");
    CONSOLE.printf("%04X\r\n", closestNetwork);

    // Update attached network if user is OK
    CONSOLE.printf("Do you want to attach to this network (y/n) ?");
    CONSOLE.transmit();
    char c;
    do
    {
        c = CONSOLE.read();
        if ((c == 'y') || (c == 'Y'))
        {
            gConfiguration.eeprom.networkId = closestNetwork;
            gConfiguration.SaveToEeprom();
            CONSOLE.printf("Now attached to network ");
            CONSOLE.printf("%04X\r\n", closestNetwork);
        }
        else
        {
            CONSOLE.printf("Attachement cancelled.\r\n");
        }
        CONSOLE.transmit();
    } while ((c != 'y') && (c != 'Y'));
}
