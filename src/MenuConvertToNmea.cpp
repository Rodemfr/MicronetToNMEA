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

void MenuConvertToNmea();
void SaveCalibration(MicronetCodec &micronetCodec);
void LoadCalibration(MicronetCodec &micronetCodec);
void ConfigureSlaveDevice(MicronetSlaveDevice &micronetDevice);

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

void MenuConvertToNmea()
{
    bool                exitNmeaLoop = false;
    MicronetMessage_t  *rxMessage;
    MicronetMessageFifo txMessageFifo;
    uint32_t            lastHeadingTime = millis();
    MicronetCodec       micronetCodec;
    DataBridge          dataBridge(&micronetCodec);
    MicronetSlaveDevice micronetDevice(&micronetCodec);

    // Check that we have been attached to a network
    if (gConfiguration.networkId == 0)
    {
        CONSOLE.println("No Micronet network has been attached.");
        CONSOLE.println("Scan and attach a Micronet network first.");
        return;
    }

    CONSOLE.println("");
    CONSOLE.println("Starting Micronet to NMEA0183 conversion.");
    CONSOLE.println("Press ESC key at any time to stop conversion and come back to menu.");
    CONSOLE.println("");

    // Load sensor calibration data into Micronet codec
    LoadCalibration(micronetCodec);

    // Configure Micronet device according to board configuration
    ConfigureSlaveDevice(micronetDevice);

    // Enable frequency tracking to keep master's frequency as reference in case of XTAL/PLL drift
    gRfReceiver.EnableFrequencyTracking(gConfiguration.networkId);

    gRxMessageFifo.ResetFifo();

    do
    {
        if ((rxMessage = gRxMessageFifo.Peek()) != nullptr)
        {
            micronetDevice.ProcessMessage(rxMessage, &txMessageFifo);
            gRfReceiver.Transmit(&txMessageFifo);

            dataBridge.UpdateMicronetData();

            if (micronetCodec.navData.calibrationUpdated)
            {
                micronetCodec.navData.calibrationUpdated = false;
                SaveCalibration(micronetCodec);
            }

            gRxMessageFifo.DeleteMessage();
        }

        while (GNSS.available() > 0)
        {
            dataBridge.PushNmeaChar(GNSS.read(), LINK_GNSS);
        }

        char c;
        while (NAV_NMEA.available() > 0)
        {
            c = NAV_NMEA.read();
            if (((void *)(&CONSOLE) == (void *)(&NAV_NMEA)) && (c == 0x1b))
            {
                CONSOLE.println("ESC key pressed, stopping conversion.");
                exitNmeaLoop = true;
            }
            dataBridge.PushNmeaChar(c, LINK_NAV);
        }

        // Only execute magnetic heading code if navigation compass is available
        if (gConfiguration.navCompassAvailable == true)
        {
            // Handle magnetic compass
            // Only request new reading if previous is at least 100ms old
            if ((millis() - lastHeadingTime) > 100)
            {
                lastHeadingTime = millis();
                dataBridge.UpdateCompassData(gNavCompass.GetHeading() + micronetCodec.navData.headingOffset_deg);
            }
        }

        if ((void *)(&CONSOLE) != (void *)(&NAV_NMEA))
        {
            while (CONSOLE.available() > 0)
            {
                if (CONSOLE.read() == 0x1b)
                {
                    CONSOLE.println("ESC key pressed, stopping conversion.");
                    exitNmeaLoop = true;
                }
            }
        }

        micronetCodec.navData.UpdateValidity();

        yield();

#if defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
        // Enter sleep mode to save power. CPU will be waken-up on next interrupt
        asm("wfi\n");
#endif

    } while (!exitNmeaLoop);

    gRfReceiver.DisableFrequencyTracking();
}

void SaveCalibration(MicronetCodec &micronetCodec)
{
    gConfiguration.waterSpeedFactor_per     = micronetCodec.navData.waterSpeedFactor_per;
    gConfiguration.waterTemperatureOffset_C = micronetCodec.navData.waterTemperatureOffset_degc;
    gConfiguration.depthOffset_m            = micronetCodec.navData.depthOffset_m;
    gConfiguration.windSpeedFactor_per      = micronetCodec.navData.windSpeedFactor_per;
    gConfiguration.windDirectionOffset_deg  = micronetCodec.navData.windDirectionOffset_deg;
    gConfiguration.headingOffset_deg        = micronetCodec.navData.headingOffset_deg;
    gConfiguration.magneticVariation_deg    = micronetCodec.navData.magneticVariation_deg;
    gConfiguration.windShift                = micronetCodec.navData.windShift_min;

    gConfiguration.SaveToEeprom();
}

void LoadCalibration(MicronetCodec &micronetCodec)
{
    micronetCodec.navData.waterSpeedFactor_per        = gConfiguration.waterSpeedFactor_per;
    micronetCodec.navData.waterTemperatureOffset_degc = gConfiguration.waterTemperatureOffset_C;
    micronetCodec.navData.depthOffset_m               = gConfiguration.depthOffset_m;
    micronetCodec.navData.windSpeedFactor_per         = gConfiguration.windSpeedFactor_per;
    micronetCodec.navData.windDirectionOffset_deg     = gConfiguration.windDirectionOffset_deg;
    micronetCodec.navData.headingOffset_deg           = gConfiguration.headingOffset_deg;
    micronetCodec.navData.magneticVariation_deg       = gConfiguration.magneticVariation_deg;
    micronetCodec.navData.windShift_min               = gConfiguration.windShift;
}

// Defines slave device parameters depending on its configuration
// @param micronetDevice MicronetSlaveDevice instance
void ConfigureSlaveDevice(MicronetSlaveDevice &micronetDevice)
{
    // Configure Micronet's slave devices
    micronetDevice.SetNetworkId(gConfiguration.networkId);
    micronetDevice.SetDeviceId(gConfiguration.deviceId);

    // All these fields are sent to Micronet whatever is the configuration
    micronetDevice.SetDataFields(DATA_FIELD_TIME | DATA_FIELD_SOGCOG | DATA_FIELD_DATE | DATA_FIELD_POSITION | DATA_FIELD_XTE | DATA_FIELD_DTW |
                                 DATA_FIELD_BTW | DATA_FIELD_VMGWP | DATA_FIELD_NODE_INFO);

    // Only send Heading to Micronet if configured so
    if (COMPASS_SOURCE_LINK != LINK_MICRONET)
    {
        micronetDevice.AddDataFields(DATA_FIELD_HDG);
    }

    // Only send depth to Micronet if configured so
    if (DEPTH_SOURCE_LINK != LINK_MICRONET)
    {
        micronetDevice.AddDataFields(DATA_FIELD_DPT);
    }

    // Only send speed to Micronet if configured so or if SPD emulation is enabled
    if ((EMULATE_SPD_WITH_SOG == 1) || (SPEED_SOURCE_LINK != LINK_MICRONET))
    {
        micronetDevice.AddDataFields(DATA_FIELD_SPD);
    }

    // Only send wind data to Micronet if configured so or if wind repeating is enabled
    if (WIND_SOURCE_LINK != LINK_MICRONET)
    {
        micronetDevice.AddDataFields(DATA_FIELD_AWS | DATA_FIELD_AWA);
    }
}
