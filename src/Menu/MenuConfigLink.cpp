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

#include "MenuConfigLink.h"
#include "BoardConfig.h"
#include "Configuration.h"
#include "Globals.h"
#include "Micronet.h"
#include "MicronetCodec.h"

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

/* No additional file-local constants required */

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/* No file-local types required */

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/**
 * PrintLinkConfig
 *
 * Display the current selection for each configurable data source and
 * the menu choices to the console.
 */
void PrintLinkConfig();

/**
 * GetLinkIndex
 *
 * Find the index of linkId inside a configuration table. Returns 0 when
 * not found which corresponds to the first element of the table.
 *
 * @param configTable Pointer to array of LinkId values
 * @param tableLength Length in bytes of configTable
 * @param linkId Link identifier to find
 * @return Index of linkId inside configTable or 0 if not found
 */
uint32_t GetLinkIndex(uint8_t *configTable, uint32_t tableLength, uint32_t linkId);

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/* Human readable names for links shown in the menu. */
const char *linkName[] = {"Plotter", "NMEA0183 Input", "Micronet", "GNSS", "Internal compass"};

/* Configuration tables define the allowed choices for each logical source.
   The index variables hold the current selection position within each table. */
uint8_t  gnssConfigTable[]    = {LINK_NMEA0183_IN, LINK_GNSS, LINK_PLOTTER};
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
       present in the choice table, GetLinkIndex returns 0 (first option). */
    gnssConfigIndex    = GetLinkIndex(gnssConfigTable, sizeof(gnssConfigTable), gConfiguration.eeprom.gnssSource);
    windConfigIndex    = GetLinkIndex(windConfigTable, sizeof(windConfigTable), gConfiguration.eeprom.windSource);
    depthConfigIndex   = GetLinkIndex(depthConfigTable, sizeof(depthConfigTable), gConfiguration.eeprom.depthSource);
    speedConfigIndex   = GetLinkIndex(speedConfigTable, sizeof(speedConfigTable), gConfiguration.eeprom.speedSource);
    compassConfigIndex = GetLinkIndex(compassConfigTable, sizeof(compassConfigTable), gConfiguration.eeprom.compassSource);

    while (!exitLoop)
    {
        PrintLinkConfig();

        do
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            c = CONSOLE.read();
        } while (c == 0xff);

        if ((c >= 0x30) && (c <= 0x39))
        {
            CONSOLE.printf("%c\r\n", c);
            CONSOLE.transmit();
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
                [[fallthrough]];
            case 6:
                /* Exit without explicit save (note case 5 falls through to exit) */
                CONSOLE.printf("Exiting to upper menu...\r\n");
                CONSOLE.transmit();
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
    CONSOLE.printf("0 - GNSS source : %s\r\n", linkName[gnssConfigTable[gnssConfigIndex]]);
    CONSOLE.printf("1 - Wind source : %s\r\n", linkName[windConfigTable[windConfigIndex]]);
    CONSOLE.printf("2 - Depth source : %s\r\n", linkName[depthConfigTable[depthConfigIndex]]);
    CONSOLE.printf("3 - Speed source : %s\r\n", linkName[speedConfigTable[speedConfigIndex]]);
    CONSOLE.printf("4 - Compass source : %s\r\n", linkName[compassConfigTable[compassConfigIndex]]);
    CONSOLE.printf("5 - Save and exit\r\n");
    CONSOLE.printf("6 - Exit without saving\r\n\r\n");
    CONSOLE.printf("Choice : ");
    CONSOLE.transmit();
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
uint32_t GetLinkIndex(uint8_t *configTable, uint32_t tableLength, uint32_t linkId)
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