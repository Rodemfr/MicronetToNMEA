/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Configure SOG/COG filtering parameters                         *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements an interactive menu to configure Speed Over      *
 * Ground (SOG) and Course Over Ground (COG) filtering parameters. It     *
 * allows users to:                                                       *
 * - Enable/disable SOG/COG filtering                                     *
 * - Adjust filter strength (time constant)                               *
 * - Enable/disable water speed emulation from SOG                        *
 *                                                                         *
 * The implementation handles parameter validation and persistence to      *
 * EEPROM when requested by the user.                                     *
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
#include "MenuConfigSogCog.h"
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
 * PrintSogCogConfig
 * 
 * Display the current SOG/COG configuration and menu options
 * to the console.
 */
void PrintSogCogConfig();

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/**
 * Working copies of SOG/COG parameters during menu interaction
 */
static bool     sogCogFilteringEnable;  // Filter enable flag
static uint32_t sogCogFilterLength;     // Filter time constant
static bool     spdEmulation;           // Water speed emulation from SOG

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuConfigSogCog
 *
 * Interactive menu to configure SOG/COG filtering parameters.
 * 
 * Allows users to:
 * - Enable/disable SOG/COG filtering
 * - Adjust filter strength (1 to SOG_COG_MAX_FILTERING_DEPTH)
 * - Enable/disable water speed emulation from SOG
 * - Save configuration to EEPROM
 * 
 * The function loads current settings from EEPROM, lets the user
 * modify them and optionally saves back to EEPROM before returning.
 */
void MenuConfigSogCog()
{
    char c;
    bool exitLoop = false;

    sogCogFilteringEnable = gConfiguration.eeprom.sogCogFilteringEnable;
    sogCogFilterLength    = gConfiguration.eeprom.sogCogFilterLength;
    spdEmulation          = gConfiguration.eeprom.spdEmulation;

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
                gConfiguration.eeprom.sogCogFilteringEnable = sogCogFilteringEnable;
                gConfiguration.eeprom.sogCogFilterLength = sogCogFilterLength;
                gConfiguration.eeprom.spdEmulation = spdEmulation;
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

/**
 * PrintSogCogConfig
 * 
 * Display the current SOG/COG settings and menu options.
 * Shows:
 * - Filter enable/disable status
 * - Current filter strength value
 * - Water speed emulation status
 * - Save and exit options
 */
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
