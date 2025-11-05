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
#include "Version.h"

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
