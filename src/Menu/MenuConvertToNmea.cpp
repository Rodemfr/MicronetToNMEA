/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Decode data from Micronet devices and forward it on an NMEA   *
 *           0183 network                                                   *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements the "Convert to NMEA" menu action. It performs:  *
 * - Initialization of conversion pipeline (Micronet codec, DataBridge)    *
 * - Loading and saving of sensor calibration data                          *
 * - Configuration of Micronet slave device parameters                      *
 * - Main loop forwarding received Micronet messages to NMEA outputs and    *
 *   handling incoming NMEA input (from plotter / NMEA IN)                  *
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

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* (No local constants defined) */

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/* (No local types defined) */

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/**
 * Entry point for the "Convert to NMEA" menu.
 * This function starts the conversion loop and returns when the user exits.
 */
void MenuConvertToNmea();

/**
 * Save calibration data from MicronetCodec into persistent configuration (EEPROM).
 *
 * @param micronetCodec Reference to configured MicronetCodec containing calibration values.
 */
void SaveCalibration(MicronetCodec &micronetCodec);

/**
 * Load calibration data from EEPROM into the MicronetCodec instance.
 *
 * @param micronetCodec Reference to MicronetCodec to populate with stored calibration values.
 */
void LoadCalibration(MicronetCodec &micronetCodec);

/**
 * Configure Micronet slave device fields according to board configuration and user settings.
 *
 * @param micronetDevice Reference to MicronetSlaveDevice to configure (network id, data fields, etc.).
 */
void ConfigureSlaveDevice(MicronetSlaveDevice &micronetDevice);

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/* (No file-local globals) */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuConvertToNmea
 *
 * Main routine executed when the user selects "Start NMEA conversion".
 * Responsibilities:
 * - Verify that the converter is attached to a Micronet network
 * - Instantiate MicronetCodec, DataBridge and MicronetSlaveDevice objects
 * - Load saved calibration and apply board-specific configuration
 * - Enter the main processing loop which:
 *     * Processes incoming Micronet messages and transmits responses
 *     * Converts updated Micronet navigation data into NMEA sentences
 *     * Forwards incoming NMEA characters from external sources to the DataBridge
 *     * Periodically samples the onboard compass if available
 *     * Saves calibration updates when notified by the codec
 *
 * The loop exits when the user presses ESC (handled via console/plotter input).
 */
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
    if (gConfiguration.eeprom.networkId == 0)
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
    gRfReceiver.EnableFrequencyTracking(gConfiguration.eeprom.networkId);

    gRxMessageFifo.ResetFifo();

    do
    {
        if ((rxMessage = gRxMessageFifo.Peek()) != nullptr)
        {
            micronetDevice.ProcessMessage(rxMessage, &txMessageFifo);
            gRfReceiver.Transmit(&txMessageFifo);

            dataBridge.SendUpdatedNMEASentences();

            if (micronetCodec.navData.calibrationUpdated)
            {
                micronetCodec.navData.calibrationUpdated = false;
                SaveCalibration(micronetCodec);
            }

            gRxMessageFifo.DeleteMessage();
        }

        while (NMEA0183_IN.available() > 0)
        {
            dataBridge.PushNmeaChar(NMEA0183_IN.read(), LINK_NMEA0183_IN);
        }

        char c;
        while (PLOTTER.available() > 0)
        {
            c = PLOTTER.read();
            if (((void *)(&CONSOLE) == (void *)(&PLOTTER)) && (c == 0x1b))
            {
                CONSOLE.println("ESC key pressed, stopping conversion.");
                exitNmeaLoop = true;
            }
            dataBridge.PushNmeaChar(c, LINK_PLOTTER);
        }

        // // Only execute magnetic heading code if navigation compass is available
        if (gConfiguration.ram.navCompassAvailable == true)
        {
            // Handle magnetic compass
            // Only request new reading if previous is at least 100ms old
            if ((millis() - lastHeadingTime) > 100)
            {
                lastHeadingTime = millis();
                float heading_deg, roll_deg;
                gNavCompass.GetHeadingAndRoll(&heading_deg, &roll_deg);
                dataBridge.UpdateCompassData(heading_deg + micronetCodec.navData.headingOffset_deg, roll_deg);
            }
        }

        if ((void *)(&CONSOLE) != (void *)(&PLOTTER))
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

    } while (!exitNmeaLoop);

    gRfReceiver.DisableFrequencyTracking();
}

/**
 * SaveCalibration
 *
 * Copies calibration-related fields from the MicronetCodec navigation data into
 * the persistent configuration structure and triggers an EEPROM write.
 *
 * @param micronetCodec Reference to the MicronetCodec instance containing the up-to-date calibration.
 */
void SaveCalibration(MicronetCodec &micronetCodec)
{
    gConfiguration.eeprom.waterSpeedFactor_per     = micronetCodec.navData.waterSpeedFactor_per;
    gConfiguration.eeprom.waterTemperatureOffset_C = micronetCodec.navData.waterTemperatureOffset_degc;
    gConfiguration.eeprom.depthOffset_m            = micronetCodec.navData.depthOffset_m;
    gConfiguration.eeprom.windSpeedFactor_per      = micronetCodec.navData.windSpeedFactor_per;
    gConfiguration.eeprom.windDirectionOffset_deg  = micronetCodec.navData.windDirectionOffset_deg;
    gConfiguration.eeprom.headingOffset_deg        = micronetCodec.navData.headingOffset_deg;
    gConfiguration.eeprom.magneticVariation_deg    = micronetCodec.navData.magneticVariation_deg;
    gConfiguration.eeprom.windShift                = micronetCodec.navData.windShift_min;

    gConfiguration.SaveToEeprom();
}

/**
 * LoadCalibration
 *
 * Restores calibration values from persistent configuration (EEPROM) into the
 * MicronetCodec navigation data structure so they are applied during conversion.
 *
 * @param micronetCodec Reference to the MicronetCodec instance to populate.
 */
void LoadCalibration(MicronetCodec &micronetCodec)
{
    micronetCodec.navData.waterSpeedFactor_per        = gConfiguration.eeprom.waterSpeedFactor_per;
    micronetCodec.navData.waterTemperatureOffset_degc = gConfiguration.eeprom.waterTemperatureOffset_C;
    micronetCodec.navData.depthOffset_m               = gConfiguration.eeprom.depthOffset_m;
    micronetCodec.navData.windSpeedFactor_per         = gConfiguration.eeprom.windSpeedFactor_per;
    micronetCodec.navData.windDirectionOffset_deg     = gConfiguration.eeprom.windDirectionOffset_deg;
    micronetCodec.navData.headingOffset_deg           = gConfiguration.eeprom.headingOffset_deg;
    micronetCodec.navData.magneticVariation_deg       = gConfiguration.eeprom.magneticVariation_deg;
    micronetCodec.navData.windShift_min               = gConfiguration.eeprom.windShift;
}

/**
 * ConfigureSlaveDevice
 *
 * Prepare the MicronetSlaveDevice structure according to saved user preferences
 * and board-specific behavior. This sets:
 * - Network and device identifiers
 * - Default set of data fields to be sent to the Micronet master
 * - Conditional inclusion of heading, depth, speed and wind fields depending on
 *   whether those sources are provided by Micronet or external inputs
 *
 * @param micronetDevice Reference to the MicronetSlaveDevice to configure.
 */
void ConfigureSlaveDevice(MicronetSlaveDevice &micronetDevice)
{
    // Configure Micronet's slave devices
    micronetDevice.SetNetworkId(gConfiguration.eeprom.networkId);
    micronetDevice.SetDeviceId(gConfiguration.eeprom.deviceId);

    // All these fields are sent to Micronet whatever is the configuration
    micronetDevice.SetDataFields(DATA_FIELD_TIME | DATA_FIELD_SOGCOG | DATA_FIELD_DATE | DATA_FIELD_POSITION | DATA_FIELD_XTE | DATA_FIELD_DTW |
                                 DATA_FIELD_BTW | DATA_FIELD_VMGWP | DATA_FIELD_NODE_INFO);

    // Only send Heading to Micronet if configured so
    if (gConfiguration.eeprom.compassSource != LINK_MICRONET)
    {
        micronetDevice.AddDataFields(DATA_FIELD_HDG);
    }

    // Only send depth to Micronet if configured so
    if (gConfiguration.eeprom.depthSource != LINK_MICRONET)
    {
        micronetDevice.AddDataFields(DATA_FIELD_DPT);
    }

    // Only send speed to Micronet if configured so or if SPD emulation is enabled
    if ((gConfiguration.eeprom.spdEmulation == 1) || (gConfiguration.eeprom.speedSource != LINK_MICRONET))
    {
        micronetDevice.AddDataFields(DATA_FIELD_SPD);
    }

    // Only send wind data to Micronet if configured so
    if (gConfiguration.eeprom.windSource != LINK_MICRONET)
    {
        micronetDevice.AddDataFields(DATA_FIELD_AWS | DATA_FIELD_AWA);
    }
}
