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

void     PrintLinkName(uint32_t linkId);
void     PrintLinkConfig();
uint32_t FindLinkIndex(uint8_t *configTable, uint32_t tableLength, uint32_t linkId);

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

const char *linkName[] = {
    "Plotter", "Internal GNSS", "Micronet", "Internal compass", "External NMEA0183",
};

uint8_t  gnssConfigTable[]    = {LINK_GNSS, LINK_AIS, LINK_PLOTTER};
uint32_t gnssConfigIndex      = 0;
uint8_t  windConfigTable[]    = {LINK_MICRONET, LINK_PLOTTER, LINK_AIS};
uint32_t windConfigIndex      = 0;
uint8_t  depthConfigTable[]   = {LINK_MICRONET, LINK_PLOTTER, LINK_AIS};
uint32_t depthConfigIndex     = 0;
uint8_t  speedConfigTable[]   = {LINK_MICRONET, LINK_PLOTTER, LINK_AIS};
uint32_t speedConfigIndex     = 0;
uint8_t  compassConfigTable[] = {LINK_COMPASS, LINK_PLOTTER, LINK_MICRONET, LINK_AIS};
uint32_t compassConfigIndex   = 0;

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

void MenuConfigLink()
{
    char c;
    bool exitLoop = false;

    gnssConfigIndex    = FindLinkIndex(gnssConfigTable, sizeof(gnssConfigTable), gConfiguration.eeprom.gnssSource);
    windConfigIndex    = FindLinkIndex(windConfigTable, sizeof(windConfigTable), gConfiguration.eeprom.windSource);
    depthConfigIndex   = FindLinkIndex(depthConfigTable, sizeof(depthConfigTable), gConfiguration.eeprom.depthSource);
    speedConfigIndex   = FindLinkIndex(speedConfigTable, sizeof(speedConfigTable), gConfiguration.eeprom.speedSource);
    compassConfigIndex = FindLinkIndex(compassConfigTable, sizeof(compassConfigTable), gConfiguration.eeprom.compassSource);

    while (!exitLoop)
    {
        PrintLinkConfig();

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
                gnssConfigIndex = (gnssConfigIndex + 1) % sizeof(gnssConfigTable);
                break;
            case 1:
                windConfigIndex = (windConfigIndex + 1) % sizeof(windConfigTable);
                break;
            case 2:
                depthConfigIndex = (depthConfigIndex + 1) % sizeof(depthConfigTable);
                break;
            case 3:
                speedConfigIndex = (speedConfigIndex + 1) % sizeof(speedConfigTable);
                break;
            case 4:
                compassConfigIndex = (compassConfigIndex + 1) % sizeof(compassConfigTable);
                break;
            case 5:
                gConfiguration.eeprom.gnssSource = (LinkId_t)gnssConfigTable[gnssConfigIndex];
                gConfiguration.eeprom.windSource = (LinkId_t)windConfigTable[windConfigIndex];
                gConfiguration.eeprom.depthSource = (LinkId_t)depthConfigTable[depthConfigIndex];
                gConfiguration.eeprom.speedSource = (LinkId_t)speedConfigTable[speedConfigIndex];
                gConfiguration.eeprom.compassSource = (LinkId_t)compassConfigTable[compassConfigIndex];
                gConfiguration.SaveToEeprom();
            case 6:
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

void PrintLinkConfig()
{
    CONSOLE.print("0 - GNSS source : ");
    PrintLinkName(gnssConfigTable[gnssConfigIndex]);
    CONSOLE.print("1 - Wind source : ");
    PrintLinkName(windConfigTable[windConfigIndex]);
    CONSOLE.print("2 - Depth source : ");
    PrintLinkName(depthConfigTable[depthConfigIndex]);
    CONSOLE.print("3 - Speed source : ");
    PrintLinkName(speedConfigTable[speedConfigIndex]);
    CONSOLE.print("4 - Compass source : ");
    PrintLinkName(compassConfigTable[compassConfigIndex]);
    CONSOLE.println("5 - Save and exit");
    CONSOLE.println("6 - Exit without saving");
    CONSOLE.println("");
    CONSOLE.print("Choice : ");
}

void PrintLinkName(uint32_t linkId)
{
    CONSOLE.println(linkName[linkId]);
}

uint32_t FindLinkIndex(uint8_t *configTable, uint32_t tableLength, uint32_t linkId)
{
    for (uint32_t i = 0; i < tableLength; i++)
    {
        if (configTable[i] == linkId)
        {
            return i;
        }
    }

    return 0;
}