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

void MenuCalibrateCompass()
{
    bool     exitLoop     = false;
    uint32_t pDisplayTime = 0;
    uint32_t pSampleTime  = 0;
    float    mx, my, mz;
    float    xMin = 1000;
    float    xMax = -1000;
    float    yMin = 1000;
    float    yMax = -1000;
    float    zMin = 1000;
    float    zMax = -1000;
    char     c;

    if (gConfiguration.navCompassAvailable == false)
    {
        CONSOLE.println("No navigation compass detected. Exiting menu ...");
        return;
    }

    CONSOLE.println("Calibrating magnetometer ... ");

    do
    {
        uint32_t currentTime = millis();
        if ((currentTime - pSampleTime) > 100)
        {
            gNavCompass.GetMagneticField(&mx, &my, &mz);
            if ((currentTime - pDisplayTime) > 250)
            {
                pDisplayTime = currentTime;
                if (mx < xMin)
                    xMin = mx;
                if (mx > xMax)
                    xMax = mx;

                if (my < yMin)
                    yMin = my;
                if (my > yMax)
                    yMax = my;

                if (mz < zMin)
                    zMin = mz;
                if (mz > zMax)
                    zMax = mz;

                CONSOLE.print("(");
                CONSOLE.print(mx);
                CONSOLE.print(" ");
                CONSOLE.print(my);
                CONSOLE.print(" ");
                CONSOLE.print(mz);
                CONSOLE.println(")");

                CONSOLE.print("[");
                CONSOLE.print((xMin + xMax) / 2);
                CONSOLE.print(" ");
                CONSOLE.print(xMax - xMin);
                CONSOLE.print("] ");
                CONSOLE.print("[");
                CONSOLE.print((yMin + yMax) / 2);
                CONSOLE.print(" ");
                CONSOLE.print(yMax - yMin);
                CONSOLE.print("] ");
                CONSOLE.print("[");
                CONSOLE.print((zMin + zMax) / 2);
                CONSOLE.print(" ");
                CONSOLE.print(zMax - zMin);
                CONSOLE.println("]");
            }
        }

        while (CONSOLE.available() > 0)
        {
            if (CONSOLE.read() == 0x1b)
            {
                CONSOLE.println("ESC key pressed, stopping scan.");
                exitLoop = true;
            }
        }
        yield();
    } while (!exitLoop);
    CONSOLE.println("Do you want to save the new calibration values (y/n) ?");
    while (CONSOLE.available() == 0)
        ;
    c = CONSOLE.read();
    if ((c == 'y') || (c == 'Y'))
    {
        gConfiguration.xMagOffset = (xMin + xMax) / 2;
        gConfiguration.yMagOffset = (yMin + yMax) / 2;
        gConfiguration.zMagOffset = (zMin + zMax) / 2;
        gConfiguration.SaveToEeprom();
        CONSOLE.println("Configuration saved");
    }
    else
    {
        CONSOLE.println("Configuration discarded");
    }
}
