/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Magnetometer calibration menu                                  *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements an interactive console menu to calibrate the     *
 * onboard/navigation magnetometer. The user is prompted to slowly rotate  *
 * the device so that the full magnetic field envelope is observed. The    *
 * code samples the raw XYZ magnetometer values, computes min/max ranges   *
 * and proposes center offsets to be saved into persistent configuration.  *
 *                                                                         *
 * Behaviour:
 *  - Samples magnetic field at regular intervals
 *  - Tracks min/max for X, Y and Z axes
 *  - Displays current sample and computed ranges on the console
 *  - Stops when the user presses ESC
 *  - Offers to save computed offsets (center of min/max) into EEPROM
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
#include "Micronet.h"
#include "MicronetCodec.h"
#include "MicronetMessageFifo.h"

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

/* No local prototypes required */

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/* No file-local globals required */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuCalibrateCompass
 *
 * Interactive magnetometer calibration routine.
 *
 * Behaviour and steps:
 *  - Verify that a navigation compass is available (gConfiguration.ram.navCompassAvailable)
 *  - Sample magnetometer values periodically (sample interval ~100 ms)
 *  - Update per-axis minimum and maximum observed values
 *  - Periodically print current sample and computed min/max ranges to console
 *  - Exit when user presses ESC
 *  - Prompt the user to save the computed offsets (center of min/max) to EEPROM
 *
 * The computed offsets are stored in:
 *   gConfiguration.eeprom.xMagOffset
 *   gConfiguration.eeprom.yMagOffset
 *   gConfiguration.eeprom.zMagOffset
 *
 * The function writes the configuration to EEPROM only if the user confirms.
 */
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

    if (gConfiguration.ram.navCompassAvailable == false)
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
        gConfiguration.eeprom.xMagOffset = (xMin + xMax) / 2;
        gConfiguration.eeprom.yMagOffset = (yMin + yMax) / 2;
        gConfiguration.eeprom.zMagOffset = (zMin + zMax) / 2;
        gConfiguration.SaveToEeprom();
        CONSOLE.println("Configuration saved");
    }
    else
    {
        CONSOLE.println("Configuration discarded");
    }
}
