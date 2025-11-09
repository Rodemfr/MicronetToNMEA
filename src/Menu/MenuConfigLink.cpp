/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Configure data source links for GNSS, wind, depth, speed     *
 *           and compass.                                                  *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements the interactive menu used to select the source   *
 * link for several logical data types (GNSS, wind, depth, speed and      *
 * compass). It allows the user to rotate through available link options,  *
 * save the selection to persistent configuration (EEPROM) and exit. The   *
 * menu is console-driven and intended for interactive configuration.      *
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
#include "MenuConfigLink.h"
#include "Micronet.h"
#include "MicronetCodec.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* No additional file-local constants required */

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/* No file-local types required */

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/**
 * PrintLinkName
 *
 * Print a human readable name for a LinkId to the console.
 *
 * @param linkId Link identifier (index into linkName array)
 */
void PrintLinkName(uint32_t linkId);

/**
 * PrintLinkConfig
 *
 * Display the current selection for each configurable data source and
 * the menu choices to the console.
 */
void PrintLinkConfig();

/**
 * FindLinkIndex
 *
 * Find the index of linkId inside a configuration table. Returns 0 when
 * not found which corresponds to the first element of the table.
 *
 * @param configTable Pointer to array of LinkId values
 * @param tableLength Length in bytes of configTable
 * @param linkId Link identifier to find
 * @return Index of linkId inside configTable or 0 if not found
 */
uint32_t FindLinkIndex(uint8_t *configTable, uint32_t tableLength, uint32_t linkId);

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/* Human readable names for links shown in the menu. */
const char *linkName[] = {"Plotter", "NMEA0183 Input", "Micronet", "Internal compass"};

/* Configuration tables define the allowed choices for each logical source.
   The index variables hold the current selection position within each table. */
uint8_t  gnssConfigTable[]    = {LINK_NMEA0183_IN, LINK_PLOTTER};
uint32_t gnssConfigIndex      = 0;
uint8_t  windConfigTable[]    = {LINK_MICRONET, LINK_PLOTTER, LINK_NMEA0183_IN};
uint32_t windConfigIndex      = 0;
uint8_t  depthConfigTable[]   = {LINK_MICRONET, LINK_PLOTTER, LINK_NMEA0183_IN};
uint32_t depthConfigIndex     = 0;
uint8_t  speedConfigTable[]   = {LINK_MICRONET, LINK_PLOTTER, LINK_NMEA0183_IN};
uint32_t speedConfigIndex     = 0;
uint8_t  compassConfigTable[] = {LINK_COMPASS, LINK_PLOTTER, LINK_MICRONET, LINK_NMEA0183_IN};
uint32_t compassConfigIndex   = 0;

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuConfigLink
 *
 * Interactive menu loop that allows the user to select which physical or
 * logical input provides GNSS, wind, depth, speed and compass data.
 *
 * Behaviour:
 *  - Initialize selection indices from gConfiguration.eeprom values
 *  - Display current configuration and prompt for a choice
 *  - Toggle selection for the chosen item (wrap-around)
 *  - Optionally save selections to EEPROM when the user chooses Save
 *  - Exit on ESC or when the user selects exit
 *
 * The function performs blocking console IO suitable for an interactive menu.
 */
void MenuConfigLink()
{
    char c;
    bool exitLoop = false;

    /* Initialize indices from stored configuration. If stored value is not
       present in the choice table, FindLinkIndex returns 0 (first option). */
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
                /* Cycle GNSS source choices */
                gnssConfigIndex = (gnssConfigIndex + 1) % sizeof(gnssConfigTable);
                break;
            case 1:
                /* Cycle wind source choices */
                windConfigIndex = (windConfigIndex + 1) % sizeof(windConfigTable);
                break;
            case 2:
                /* Cycle depth source choices */
                depthConfigIndex = (depthConfigIndex + 1) % sizeof(depthConfigTable);
                break;
            case 3:
                /* Cycle speed source choices */
                speedConfigIndex = (speedConfigIndex + 1) % sizeof(speedConfigTable);
                break;
            case 4:
                /* Cycle compass source choices */
                compassConfigIndex = (compassConfigIndex + 1) % sizeof(compassConfigTable);
                break;
            case 5:
                /* Save selections into persistent configuration */
                gConfiguration.eeprom.gnssSource    = (LinkId_t)gnssConfigTable[gnssConfigIndex];
                gConfiguration.eeprom.windSource    = (LinkId_t)windConfigTable[windConfigIndex];
                gConfiguration.eeprom.depthSource   = (LinkId_t)depthConfigTable[depthConfigIndex];
                gConfiguration.eeprom.speedSource   = (LinkId_t)speedConfigTable[speedConfigIndex];
                gConfiguration.eeprom.compassSource = (LinkId_t)compassConfigTable[compassConfigIndex];
                gConfiguration.SaveToEeprom();
                break;
            case 6:
                /* Exit without explicit save (note case 5 falls through to exit) */
                CONSOLE.println("Exiting to upper menu...");
                return;
            }
        }
        else if (c == 0x1b)
        {
            /* ESC pressed: exit menu without saving */
            exitLoop = true;
        }
    }
}

/**
 * PrintLinkConfig
 *
 * Display the menu lines showing current selection for each logical source
 * and the numeric choices for the user.
 */
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

/**
 * PrintLinkName
 *
 * Print a readable link name corresponding to the provided LinkId.
 *
 * @param linkId Index into the linkName array
 */
void PrintLinkName(uint32_t linkId)
{
    CONSOLE.println(linkName[linkId]);
}

/**
 * FindLinkIndex
 *
 * Search configTable for linkId and return the index into the table.
 * If not found, return 0 which corresponds to the first table element.
 *
 * @param configTable Pointer to the configuration table (array of LinkId)
 * @param tableLength Length in bytes of configTable
 * @param linkId Link identifier to locate
 * @return Index of found entry or 0 if not found
 */
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