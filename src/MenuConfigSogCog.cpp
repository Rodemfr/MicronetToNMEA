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
#include "MenuConfigLink.h"
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

void PrintSogCogConfig();

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

static bool     sogCogFilteringEnable;
static uint32_t sogCogFilterLength;
static bool     spdEmulation;

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

void MenuConfigSogCog()
{
    char c;
    bool exitLoop = false;

    sogCogFilteringEnable = gConfiguration.sogCogFilteringEnable;
    sogCogFilterLength    = gConfiguration.sogCogFilterLength;
    spdEmulation          = gConfiguration.spdEmulation;

    while (!exitLoop)
    {
        PrintSogCogConfig();

        do
        {
            yield();
            c = CONSOLE.read();
        } while (c == 0xff);

        if ((c >= 0x30) && (c <= 0x39))
        {
            CONSOLE.println(c);
            c -= 0x30;
            switch (c)
            {
            case 0:
                sogCogFilteringEnable = !sogCogFilteringEnable;
                break;
            case 1:
                sogCogFilterLength = (sogCogFilterLength + 1) % SOG_COG_MAX_FILTERING_DEPTH;
                if (sogCogFilterLength <= 0) {
                    sogCogFilterLength = 1;
                }
                break;
            case 2:
                spdEmulation = !spdEmulation;
                break;
            case 3:
                gConfiguration.sogCogFilteringEnable = sogCogFilteringEnable;
                gConfiguration.sogCogFilterLength = sogCogFilterLength;
                gConfiguration.spdEmulation = spdEmulation;
                gConfiguration.SaveToEeprom();
            case 4:
                CONSOLE.println("Exiting to upper menu...");
                return;
            }
        }
        else if (c == 0x1b)
        {
            exitLoop = true;
        }
    }
}

void PrintSogCogConfig()
{
    CONSOLE.println("");
    CONSOLE.print("*** ");
    CONSOLE.print("SOG/COG Configuration");
    CONSOLE.println(" ***");
    CONSOLE.println("");
    CONSOLE.print("0 - SOG/COG Filter : ");
    if (sogCogFilteringEnable)
    {
        CONSOLE.println("ENABLED");
    }
    else
    {
        CONSOLE.println("DISABLED");
    }
    CONSOLE.print("1 - Filter strength : ");
    CONSOLE.println(sogCogFilterLength);
    CONSOLE.print("2 - Emulate water speed with SOG : ");
    if (spdEmulation)
    {
        CONSOLE.println("ENABLED");
    }
    else
    {
        CONSOLE.println("DISABLED");
    }
    CONSOLE.println("3 - Save and exit");
    CONSOLE.println("4 - Exit without saving");
    CONSOLE.println("");
    CONSOLE.print("Choice : ");
}
