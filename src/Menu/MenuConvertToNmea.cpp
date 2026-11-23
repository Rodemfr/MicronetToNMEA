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

#include "BoardConfig.h"
#include "Configuration.h"
#include "DataBridge.h"
#include "Globals.h"
#include "Micronet.h"
#include "MicronetCodec.h"
#include "MicronetMessageFifo.h"
#include "MicronetSlaveDevice.h"

extern "C"
{
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
}

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

void MenuConvertToNmea();
void SaveCalibration(MicronetCodec &micronetCodec);
void LoadCalibration(MicronetCodec &micronetCodec);
void ConfigureSlaveDevice(MicronetSlaveDevice &micronetDevice);
void TaskI2cReader(void *pvParameters);
void TaskNmeaInReader(void *pvParameters);
void TaskGnssReader(void *pvParameters);
void TaskMicronet(void *pvParameters);

struct MicronetTaskParameters
{
    DataBridge          *dataBridge;
    MicronetSlaveDevice *micronetDevice;
    MicronetMessageFifo *txMessageFifo;
};

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
    TaskHandle_t        i2cReaderTaskHandle    = nullptr;
    TaskHandle_t        nmeaInReaderTaskHandle = nullptr;
    TaskHandle_t        gnssReaderTaskHandle   = nullptr;
    TaskHandle_t        micronetTaskHandle     = nullptr;
    bool                exitNmeaLoop           = false;
    MicronetMessageFifo txMessageFifo;
    MicronetCodec       micronetCodec;
    DataBridge          dataBridge(&micronetCodec);
    MicronetSlaveDevice micronetDevice(&micronetCodec);
    MicronetTaskParameters micronetTaskParameters = {&dataBridge, &micronetDevice, &txMessageFifo};

    // Check that we have been attached to a network
    if (gConfiguration.eeprom.networkId == 0)
    {
        CONSOLE.printf("No Micronet network has been attached.\r\n");
        CONSOLE.printf("Scan and attach a Micronet network first.\r\n");
        CONSOLE.transmit();
        return;
    }

    // Load sensor calibration data into Micronet codec
    LoadCalibration(micronetCodec);

    // Configure Micronet device according to board configuration
    ConfigureSlaveDevice(micronetDevice);

    // Enable frequency tracking to keep master's frequency as reference in case of XTAL/PLL drift
    gRfReceiver.EnableFrequencyTracking(gConfiguration.eeprom.networkId);

    gRxMessageFifo.ResetFifo();

    xTaskCreate(TaskI2cReader, "TaskI2cReader", 4096, &dataBridge, tskIDLE_PRIORITY + 1, &i2cReaderTaskHandle);
    xTaskCreate(TaskNmeaInReader, "TaskNmeaInReader", 4096, &dataBridge, tskIDLE_PRIORITY + 1, &nmeaInReaderTaskHandle);
    xTaskCreate(TaskGnssReader, "TaskGnssReader", 4096, &dataBridge, tskIDLE_PRIORITY + 1, &gnssReaderTaskHandle);
    xTaskCreate(TaskMicronet, "TaskMicronet", 4096, &micronetTaskParameters, tskIDLE_PRIORITY + 2, &micronetTaskHandle);

    if ((i2cReaderTaskHandle == nullptr) || (nmeaInReaderTaskHandle == nullptr) || (gnssReaderTaskHandle == nullptr) ||
        (micronetTaskHandle == nullptr))
    {
        CONSOLE.printf("Failed to create a conversion task.\r\n");
        CONSOLE.transmit();
        if (i2cReaderTaskHandle != nullptr)
        {
            vTaskDelete(i2cReaderTaskHandle);
        }
        if (nmeaInReaderTaskHandle != nullptr)
        {
            vTaskDelete(nmeaInReaderTaskHandle);
        }
        if (gnssReaderTaskHandle != nullptr)
        {
            vTaskDelete(gnssReaderTaskHandle);
        }
        if (micronetTaskHandle != nullptr)
        {
            vTaskDelete(micronetTaskHandle);
        }
        gRfReceiver.DisableFrequencyTracking();
        return;
    }

    CONSOLE.printf("\r\nStarting Micronet to NMEA0183 conversion.\r\n");
    CONSOLE.printf("Press ESC key at any time to stop conversion and come back to menu.\r\n");
    CONSOLE.transmit();

    do
    {
        char c;
        while (PLOTTER.available() > 0)
        {
            c = PLOTTER.read();
            if (((void *)(&CONSOLE) == (void *)(&PLOTTER)) && (c == 0x1b))
            {
                CONSOLE.printf("ESC key pressed, stopping conversion.\r\n");
                CONSOLE.transmit();
                exitNmeaLoop = true;
            }
            dataBridge.PushNmeaChar(c, LINK_PLOTTER);
        }

        if ((void *)(&CONSOLE) != (void *)(&PLOTTER))
        {
            while (CONSOLE.available() > 0)
            {
                if (CONSOLE.read() == 0x1b)
                {
                    CONSOLE.printf("ESC key pressed, stopping conversion.\r\n");
                    CONSOLE.transmit();
                    exitNmeaLoop = true;
                }
            }
        }

        dataBridge.UpdateDataValidity();

        vTaskDelay(pdMS_TO_TICKS(10));

    } while (!exitNmeaLoop);

    gRfReceiver.DisableFrequencyTracking();

    if (micronetTaskHandle != nullptr)
    {
        vTaskDelete(micronetTaskHandle);
    }
    if (gnssReaderTaskHandle != nullptr)
    {
        vTaskDelete(gnssReaderTaskHandle);
    }
    if (nmeaInReaderTaskHandle != nullptr)
    {
        vTaskDelete(nmeaInReaderTaskHandle);
    }
    if (i2cReaderTaskHandle != nullptr)
    {
        vTaskDelete(i2cReaderTaskHandle);
    }
}

/**
 * TaskI2cReader
 *
 * Placeholder I2C reader task that periodically yields to the scheduler.
 *
 * @param pvParameters Pointer to the DataBridge instance.
 */
void TaskI2cReader(void *pvParameters)
{
    DataBridge *dataBridge = static_cast<DataBridge *>(pvParameters);

    while (true)
    {
        // Only execute magnetic heading code if navigation compass is available
        if (gConfiguration.ram.navCompassAvailable == true)
        {
            float heading_deg, roll_deg;
            gNavCompass.GetHeadingAndRoll(&heading_deg, &roll_deg);
            dataBridge->UpdateCompassData(heading_deg, roll_deg);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * TaskNmeaInReader
 *
 * Reads NMEA sentences from the NMEA0183 input and forwards their characters
 * to the DataBridge.
 *
 * @param pvParameters Pointer to the DataBridge instance.
 */
void TaskNmeaInReader(void *pvParameters)
{
    DataBridge *dataBridge = static_cast<DataBridge *>(pvParameters);

    while (true)
    {
        while (NMEA0183_IN.available() > 0)
        {
            dataBridge->PushNmeaChar(NMEA0183_IN.read(), LINK_NMEA0183_IN);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * TaskGnssReader
 *
 * Reads NMEA sentences from the GNSS receiver and forwards their characters
 * to the DataBridge.
 *
 * @param pvParameters Pointer to the DataBridge instance.
 */
void TaskGnssReader(void *pvParameters)
{
    DataBridge *dataBridge = static_cast<DataBridge *>(pvParameters);

    while (true)
    {
        while (GNSS.available() > 0)
        {
            dataBridge->PushNmeaChar(GNSS.read(), LINK_GNSS);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * TaskMicronet
 *
 * Processes received Micronet messages and sends updated NMEA sentences.
 *
 * @param pvParameters Pointer to the Micronet task parameters.
 */
void TaskMicronet(void *pvParameters)
{
    MicronetTaskParameters *parameters = static_cast<MicronetTaskParameters *>(pvParameters);
    MicronetCodec          *micronetCodec = parameters->dataBridge->GetMicronetCodec();

    while (true)
    {
        MicronetMessage_t rxMessage;
        if (gRxMessageFifo.PopWait(&rxMessage))
        {
            parameters->micronetDevice->ProcessMessage(&rxMessage, parameters->txMessageFifo);
            gRfReceiver.Transmit(parameters->txMessageFifo);
            parameters->dataBridge->SendUpdatedNMEASentences();

            if (micronetCodec->navData.calibrationUpdated)
            {
                micronetCodec->navData.calibrationUpdated = false;
                SaveCalibration(*micronetCodec);
            }
        }

    }
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
