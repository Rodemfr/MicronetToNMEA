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
#include "MenuConfigSogCog.h"
#include "MenuConfigOrient.h"
#include "MenuConfigMtn.h"
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

void ConfigExit();

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

bool        exitConfig;
MenuEntry_t configMenuDesc[] = {{"Configuration", nullptr},
                                {"Configure data sources", MenuConfigLink},
                                {"Configure SOG/COG", MenuConfigSogCog},
                                {"Configure orientation", MenuConfigOrientation},
                                {"Return to main menu", ConfigExit},
                                {nullptr, nullptr}};

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

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

void ConfigLink()
{
}

void ConfigSogCog()
{
}

void ConfigOrientation()
{
}

void ConfigExit()
{
    exitConfig = true;
}
