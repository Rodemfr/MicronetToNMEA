/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Main configuration menu handler for MicronetToNMEA            *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements the main configuration menu that provides access *
 * to all device settings. It manages a console-based menu system with    *
 * options to:                                                            *
 * - Configure data source links (NMEA, Micronet, internal sensors)       *
 * - Configure SOG/COG filtering and computation                          *
 * - Configure device orientation and calibration                         *
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
#include "MenuConfigSogCog.h"
#include "MenuConfigOrient.h"
#include "MenuConfigMtn.h"
#include "Micronet.h"
#include "MicronetCodec.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* No file-local constants required */

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/* No file-local types required */

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/**
 * ConfigExit
 * 
 * Callback invoked when the user selects "Return to main menu".
 * Sets exitConfig flag to true to terminate the menu loop.
 */
void ConfigExit();

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/**
 * Flag to control menu loop execution
 * Set to true by ConfigExit() to terminate the menu
 */
bool exitConfig;

/**
 * Menu structure definition
 * Each entry contains a display string and a callback function
 * nullptr callback indicates menu title or end of menu
 */
MenuEntry_t configMenuDesc[] = {
    {"Configuration", nullptr},
    {"Configure data sources", MenuConfigLink},
    {"Configure SOG/COG", MenuConfigSogCog},
    {"Configure orientation", MenuConfigOrientation},
    {"Return to main menu", ConfigExit},
    {nullptr, nullptr}
};

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuConfigMtn
 * 
 * Main configuration menu entry point. Creates and runs a MenuManager instance
 * that handles the console-based menu system. Processes user input until
 * exitConfig is set to true by the ConfigExit callback.
 */
void MenuConfigMtn()
{
    MenuManager configMenuManager;

    configMenuManager.SetMenuDescription(configMenuDesc);
    configMenuManager.PrintMenu();
    configMenuManager.PrintPrompt();
    exitConfig = false;

    while (!exitConfig)
    {
        // Process console input
        while (CONSOLE.available() > 0)
        {
            configMenuManager.PushChar(CONSOLE.read());
        }

        yield();
    }
}

/**
 * ConfigLink
 * 
 * Placeholder for data source configuration callback
 * Implemented in MenuConfigLink.cpp
 */
void ConfigLink()
{
}

/**
 * ConfigSogCog
 * 
 * Placeholder for SOG/COG configuration callback
 * Implemented in MenuConfigSogCog.cpp
 */
void ConfigSogCog()
{
}

/**
 * ConfigOrientation
 * 
 * Placeholder for orientation configuration callback
 * Implemented in MenuConfigOrient.cpp
 */
void ConfigOrientation()
{
}

/**
 * ConfigExit
 * 
 * Menu callback that sets exitConfig to true, causing the menu loop
 * to terminate and return to the main menu.
 */
void ConfigExit()
{
    exitConfig = true;
}
