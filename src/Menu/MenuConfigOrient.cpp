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
#include "MenuConfigOrient.h"
#include "Micronet.h"
#include "MicronetCodec.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define NB_AXIS 6

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

void PrintOrientationConfig();

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

static uint32_t headingAxis;
static uint32_t downAxis;

static const char *axisName[NB_AXIS] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

void MenuConfigOrientation()
{
    char c;
    bool exitLoop = false;

    headingAxis = gConfiguration.eeprom.headingAxis;
    downAxis    = gConfiguration.eeprom.downAxis;

    while (!exitLoop)
    {
        PrintOrientationConfig();

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
                do
                {
                    headingAxis = (headingAxis + 1) % NB_AXIS;
                } while (headingAxis == downAxis);
                break;
            case 1:
                do
                {
                    downAxis = (downAxis + 1) % NB_AXIS;
                } while (downAxis == headingAxis);
                break;
            case 2:
                gConfiguration.eeprom.headingAxis = (Axis_t)headingAxis;
                gConfiguration.eeprom.downAxis    = (Axis_t)downAxis;
                gConfiguration.SaveToEeprom();
            case 3:
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

void PrintOrientationConfig()
{
    CONSOLE.println("");
    CONSOLE.print("*** ");
    CONSOLE.print("Compass orientation");
    CONSOLE.println(" ***");
    CONSOLE.println("");
    CONSOLE.print("0 - Heading/bow axis : ");
    CONSOLE.println(axisName[headingAxis]);
    CONSOLE.print("1 - Bottom/down axis : ");
    CONSOLE.println(axisName[downAxis]);
    CONSOLE.println("2 - Save and exit");
    CONSOLE.println("3 - Exit without saving");
    CONSOLE.println("");
    CONSOLE.print("Choice : ");
}
