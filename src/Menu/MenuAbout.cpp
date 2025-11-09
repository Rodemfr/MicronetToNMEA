/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Provide "About" menu entry that shows device and configuration*
 *           status information on the console.                            *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements the menu action which prints firmware version,    *
 * persistent configuration status and runtime-detected hardware state to  *
 * the console. It is intended for diagnostic and configuration checks by   *
 * the user.                                                               *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021 by Ronan Demoment                                  *
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
#include "Version.h"

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
 * MenuAbout
 *
 * Print firmware version, EEPROM configuration status and runtime hardware
 * detection results to the console. This function is called from the menu
 * system when the user selects the "About" entry.
 *
 * The printed information includes:
 *  - Software version (major.minor)
 *  - Whether a valid configuration was found in EEPROM and its checksum status
 *  - Device ID and attached Micronet network ID if any
 *  - RF frequency offset and its ppm equivalent
 *  - Calibration and offsets for wind, water speed & temperature
 *  - Compass detection state and magnetometer calibration values if available
 *
 * The function does not alter state, it only reports information for the user.
 */
void MenuAbout()
{
    CONSOLE.print("MicronetToNMEA, Version ");
    CONSOLE.print(MNET2NMEA_SW_MAJOR_VERSION, DEC);
    CONSOLE.print(".");
    CONSOLE.println(MNET2NMEA_SW_MINOR_VERSION, DEC);

    if (!gConfiguration.magicNumberFound)
    {
        CONSOLE.println("Configuration not found in EEPROM");
    }
    else
    {
        if (gConfiguration.checksumValid)
        {
            CONSOLE.println("Valid configuration found in EEPROM");
        }
        else
        {
            CONSOLE.println("Invalid configuration found in EEPROM");
        }
    }

    CONSOLE.print("Device ID : ");
    CONSOLE.println(gConfiguration.eeprom.deviceId, HEX);

    if (gConfiguration.eeprom.networkId != 0)
    {
        CONSOLE.print("Attached to Micronet Network ");
        CONSOLE.println(gConfiguration.eeprom.networkId, HEX);
    }
    else
    {
        CONSOLE.println("No Micronet Network attached");
    }

    CONSOLE.print("RF Frequency offset = ");
    CONSOLE.print(gConfiguration.eeprom.rfFrequencyOffset_MHz * 1000);
    CONSOLE.print(" kHz (");
    CONSOLE.print((int)(1000000.0 * gConfiguration.eeprom.rfFrequencyOffset_MHz / MICRONET_RF_CENTER_FREQUENCY_MHZ));
    CONSOLE.println(" ppm)");
    CONSOLE.print("Wind speed factor = ");
    CONSOLE.println(gConfiguration.eeprom.windSpeedFactor_per);
    CONSOLE.print("Wind direction offset = ");
    CONSOLE.println((int)(gConfiguration.eeprom.windDirectionOffset_deg));
    CONSOLE.print("Water speed factor = ");
    CONSOLE.println(gConfiguration.eeprom.waterSpeedFactor_per);
    CONSOLE.print("Water temperature offset = ");
    CONSOLE.println((int)(gConfiguration.eeprom.waterTemperatureOffset_C));
    if (gConfiguration.ram.navCompassAvailable == false)
    {
        CONSOLE.println("No navigation compass detected, disabling magnetic heading.");
    }
    else
    {
        CONSOLE.print("Using ");
        CONSOLE.print(gNavCompass.GetDeviceName().c_str());
        CONSOLE.println(" for magnetic heading");
        CONSOLE.print("Magnetometer calibration : ");
        CONSOLE.print(gConfiguration.eeprom.xMagOffset);
        CONSOLE.print(" ");
        CONSOLE.print(gConfiguration.eeprom.yMagOffset);
        CONSOLE.print(" ");
        CONSOLE.println(gConfiguration.eeprom.zMagOffset);
        CONSOLE.print("Heading offset = ");
        CONSOLE.println((int)(gConfiguration.eeprom.headingOffset_deg));
        CONSOLE.print("Magnetic variation = ");
        CONSOLE.println((int)(gConfiguration.eeprom.magneticVariation_deg));
        CONSOLE.print("Depth offset = ");
        CONSOLE.println(gConfiguration.eeprom.depthOffset_m);
        CONSOLE.print("Wind shift = ");
        CONSOLE.println((int)gConfiguration.eeprom.windShift);
    }
}
