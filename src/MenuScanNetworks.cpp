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
#include "MicronetMessageFifo.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define MAX_SCANNED_NETWORKS 5

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

void MenuScanNetworks()
{
    MicronetMessage_t *message;
    uint32_t           nidArray[MAX_SCANNED_NETWORKS];
    int16_t            rssiArray[MAX_SCANNED_NETWORKS];
    MicronetCodec      micronetCodec;

    memset(nidArray, 0, sizeof(nidArray));
    memset(rssiArray, 0, sizeof(rssiArray));

    CONSOLE.print("Scanning Micronet networks for 5 seconds ... ");

    gRxMessageFifo.ResetFifo();
    unsigned long startTime = millis();
    do
    {
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
                            for (int j = (MAX_SCANNED_NETWORKS - 1); j > i; j++)
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
    } while ((millis() - startTime) < 5000);

    CONSOLE.println("done");
    CONSOLE.println("");

    // Print result
    if (nidArray[0] != 0)
    {
        CONSOLE.println("List of scanned networks :");
        CONSOLE.println("");
        for (int i = 0; i < MAX_SCANNED_NETWORKS; i++)
        {
            if (nidArray[i] != 0)
            {
                CONSOLE.print("Network ");
                CONSOLE.print(i);
                CONSOLE.print(" - ");
                CONSOLE.print(nidArray[i], HEX);
                CONSOLE.print(" (");
                if (rssiArray[i] < 70)
                    CONSOLE.print("very strong");
                else if (rssiArray[i] < 80)
                    CONSOLE.print("strong");
                else if (rssiArray[i] < 90)
                    CONSOLE.print("normal");
                else
                    CONSOLE.print("low");
                CONSOLE.println(")");
            }
        }
    }
    else
    {
        CONSOLE.println("/!\\ No Micronet network found /!\\");
        CONSOLE.println("Check that your Micronet network is powered on.");
    }
}

