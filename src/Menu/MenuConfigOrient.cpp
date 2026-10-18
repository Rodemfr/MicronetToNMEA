/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Configure compass orientation and mounting position            *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements an interactive menu to configure how the         *
 * compass/magnetometer is mounted in the device. It allows users to       *
 * specify which axes represent:                                          *
 * - The heading/bow direction (forward direction of the boat)            *
 * - The down direction (vertical alignment)                              *
 *                                                                         *
 * The implementation handles axis selection, validates that heading and   *
 * down axes are different, and persists settings to EEPROM when          *
 * requested by the user.                                                 *
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

#include "MenuConfigOrient.h"
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

/**
 * Number of possible axes orientations (+X, -X, +Y, -Y, +Z, -Z)
 */
#define NB_AXIS 6

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/* No local types required */

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/**
 * PrintOrientationConfig
 *
 * Display the current orientation configuration and menu options
 * to the console.
 */
void PrintOrientationConfig();

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/**
 * Working copies of axis configuration during menu interaction
 */
static uint32_t headingAxis;
static uint32_t downAxis;

/**
 * Human readable names for the 6 possible axis orientations
 */
static const char *axisName[NB_AXIS] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuConfigOrientation
 *
 * Interactive menu to configure compass mounting orientation.
 *
 * Allows users to:
 * - Select which axis represents the heading/bow direction
 * - Select which axis represents the down direction
 * - Save configuration to EEPROM
 *
 * The function ensures heading and down axes remain different
 * while cycling through options. Returns when user saves or exits.
 */
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
                [[fallthrough]];
            case 3:
                CONSOLE.printf("Exiting to upper menu...");
                CONSOLE.transmit();
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
 * PrintOrientationConfig
 *
 * Display the current orientation settings and menu options.
 * Shows:
 * - Current heading/bow axis selection
 * - Current bottom/down axis selection
 * - Save and exit options
 */
void PrintOrientationConfig()
{
    CONSOLE.printf("\r\n*** Compass orientation ***\r\n\r\n");
    CONSOLE.printf("0 - Heading/bow axis : %s\r\n", axisName[headingAxis]);
    CONSOLE.printf("1 - Bottom/down axis : %s\r\n", axisName[downAxis]);
    CONSOLE.printf("2 - Save and exit\r\n");
    CONSOLE.printf("3 - Exit without saving\r\n\r\n");
    CONSOLE.printf("Choice : ");
    CONSOLE.transmit();
}
