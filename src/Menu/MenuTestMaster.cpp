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
#include "MenuTestMaster.h"
#include "Micronet.h"
#include "MicronetCodec.h"
#include "MicronetMessageFifo.h"

extern void PrintNetworkMap(MicronetCodec::NetworkMap *networkMap);
extern void PrintRawMessage(MicronetMessage_t *message, uint32_t lastMasterRequest_us);
extern void PrintByte(uint8_t data);
extern void PrintInt(uint32_t data);

static uint32_t nbSlaves    = 6;
static uint32_t slaveIds[]  = {0x81037082, 0x81071E60, 0x870B85B4, 0x01071E77, 0x83037737, 0x830AB252};
static uint8_t  slaveLens[] = {0x21, 0x0, 0x06, 0x0c, 0x00, 0x00};

void MenuTestMaster()
{
    bool               exitNmeaLoop = false;
    MicronetMessage_t *rxMessage;
    MicronetMessage_t  txMessage;
    uint32_t           lastHeadingTime = millis();
    MicronetCodec      micronetCodec;

    CONSOLE.println("");
    CONSOLE.println("Starting Micronet Master Test.");
    CONSOLE.println("Press ESC key at any time to stop test and come back to menu.");
    CONSOLE.println("");

    gConfiguration.LoadFromEeprom();
    gRxMessageFifo.ResetFifo();

    delay(5000);
    // gRfReceiver.StartCWTransmit();
    // delay(5000);
    // gRfReceiver.StopCWTransmit();
    gRfReceiver.StartCWSweep();
    gRfReceiver.RestartReception();

    uint32_t nextNetworkCycle_us = micros() + 1000000;

    do
    {
        if (micros() > nextNetworkCycle_us - 100000)
        {
            micronetCodec.EncodeMasterRequest(&txMessage, 0, gConfiguration.eeprom.networkId, 0x81037082, nbSlaves, slaveIds, slaveLens);
            txMessage.action       = MICRONET_ACTION_RF_TRANSMIT;
            txMessage.startTime_us = nextNetworkCycle_us;
            gRfReceiver.Transmit(&txMessage);
            nextNetworkCycle_us += 1000000;
            PrintRawMessage(&txMessage, 0);
        }

        while (CONSOLE.available() > 0)
        {
            if (CONSOLE.read() == 0x1b)
            {
                CONSOLE.println("ESC key pressed, stopping conversion.");
                exitNmeaLoop = true;
            }
        }

        yield();
    } while (!exitNmeaLoop);
}
