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

bool isResponseOk();
void PrintResponse();

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

void MenuConfigBt()
{
    const int   baudRateTable[] = {9600, 19200, 38400, 57600, 115200};
    const char *baudStrings[]   = {"AT+BAUD0", "AT+BAUD1", "AT+BAUD2", "AT+BAUD3", "AT+BAUD4"};
    const char *sppNameCommand  = "AT+SPNAMicronetToNMEA";
    const char *bleNameCommand  = "AT+LENAMicronetToNMEA";
    int         actualBaudrate  = 0;

    CONSOLE.println("Detecting bluetooth module ... ");

    for (uint i = 0; i < (sizeof(baudRateTable) / sizeof(int)); i++)
    {
        // Reconfigure UART speed
        PLOTTER.end();
        PLOTTER.begin(baudRateTable[i]);
        delay(100);
        // Request switch to command mode
        PLOTTER.println("AT+ENAT");
        delay(100);
        if (isResponseOk())
        {
            CONSOLE.print("VG 6328A found with baudrate ");
            CONSOLE.println(baudRateTable[i]);
            actualBaudrate = baudRateTable[i];
            break;
        }
    }

    if (actualBaudrate == 0)
    {
        CONSOLE.println("Bluetooth module not found, aborting configuration.");
        return;
    }

    if (actualBaudrate != PLOTTER_BAUDRATE)
    {
        for (uint i = 0; i < (sizeof(baudRateTable) / sizeof(int)); i++)
        {
            if (baudRateTable[i] == PLOTTER_BAUDRATE)
            {
                CONSOLE.print("Configuring baudrate to ");
                CONSOLE.print(baudRateTable[i]);
                CONSOLE.println(" baud");
                // Request switch to command mode
                PLOTTER.println(baudStrings[i]);
                delay(100);
                PLOTTER.clear();
                break;
            }
        }
    }
    CONSOLE.println("Changing device name to MicronetToNMEA");
    PLOTTER.println(sppNameCommand);
    delay(500);
    PLOTTER.println(bleNameCommand);
    delay(500);
    CONSOLE.println("Switching OFF BLE");
    PLOTTER.println("AT+LEOF");
    delay(500);
    CONSOLE.println("Switching ON SPP");
    PLOTTER.println("AT+SPON");
    delay(500);
    PLOTTER.print("AT+REST");
    delay(500);
    PLOTTER.clear();
}

bool isResponseOk()
{
    char response[5];
    int  nbChars = PLOTTER.available();
    if (nbChars == 4)
    {
        response[0] = (char)PLOTTER.read();
        response[1] = (char)PLOTTER.read();
        response[2] = (char)PLOTTER.read();
        response[3] = (char)PLOTTER.read();
        response[4] = 0;
        PLOTTER.clear();
        if (!strcmp(response, "OK\r\n"))
        {
            return true;
        }
    }

    return false;
}

void PrintResponse()
{
    while (PLOTTER.available() > 0)
    {
        CONSOLE.print((char)PLOTTER.read());
    }
}
