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
    CONSOLE.printf("MicronetToNMEA, Version %d.%d\r\n", MNET2NMEA_SW_MAJOR_VERSION, MNET2NMEA_SW_MINOR_VERSION);

    if (!gConfiguration.magicNumberFound)
    {
        CONSOLE.printf("Configuration not found in EEPROM\r\n");
    }
    else
    {
        if (gConfiguration.checksumValid)
        {
            CONSOLE.printf("Valid configuration found in EEPROM\r\n");
        }
        else
        {
            CONSOLE.printf("Invalid configuration found in EEPROM\r\n");
        }
    }

    CONSOLE.printf("Device ID : %08X\r\n", gConfiguration.eeprom.deviceId);

    if (gConfiguration.eeprom.networkId != 0)
    {
        CONSOLE.printf("Attached to Micronet Network %08x\r\n", gConfiguration.eeprom.networkId);
    }
    else
    {
        CONSOLE.printf("No Micronet Network attached\r\n");
    }

    CONSOLE.printf("RF Frequency offset = %d kHz (%d ppm)\r\n", (int)(gConfiguration.eeprom.rfFrequencyOffset_MHz * 1000),
                   (int)(1000000.0 * gConfiguration.eeprom.rfFrequencyOffset_MHz / MICRONET_RF_CENTER_FREQUENCY_MHZ));
    CONSOLE.printf("Wind speed factor = %d\r\n", gConfiguration.eeprom.windSpeedFactor_per);
    CONSOLE.printf("Wind direction offset = %d\r\n", (int)(gConfiguration.eeprom.windDirectionOffset_deg));
    CONSOLE.printf("Water speed factor = %d\r\n", gConfiguration.eeprom.waterSpeedFactor_per);
    CONSOLE.printf("Water temperature offset = %d\r\n", (int)(gConfiguration.eeprom.waterTemperatureOffset_C));
    if (gConfiguration.ram.navCompassAvailable == false)
    {
        CONSOLE.printf("No navigation compass detected, disabling magnetic heading.\r\n");
    }
    else
    {
        CONSOLE.printf("Using %s for magnetic heading\r\n", gNavCompass.GetDeviceName().c_str());
        CONSOLE.printf("Magnetometer calibration : %d %d %d\r\n", gConfiguration.eeprom.xMagOffset, gConfiguration.eeprom.yMagOffset,
                       gConfiguration.eeprom.zMagOffset);
        CONSOLE.printf("Heading offset = %d\r\n", (int)(gConfiguration.eeprom.headingOffset_deg));
        CONSOLE.printf("Magnetic variation = %d\r\n", (int)(gConfiguration.eeprom.magneticVariation_deg));
        CONSOLE.printf("Depth offset = %d\r\n", gConfiguration.eeprom.depthOffset_m);
        CONSOLE.printf("Wind shift = %d\r\n", (int)gConfiguration.eeprom.windShift);
    }
}