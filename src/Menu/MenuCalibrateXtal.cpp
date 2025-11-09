/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  RF frequency tuning utility for Micronet receiver             *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements an interactive console routine used to tune the  *
 * RF oscillator of the CC1101-based receiver so the device aligns with   *
 * the Micronet master transmitter frequency. The procedure performs a     *
 * frequency sweep around the nominal center frequency, counts received    *
 * master requests and derives the working frequency range. If a valid    *
 * range is found, the user may save the computed frequency offset into    *
 * persistent configuration (EEPROM).                                      *
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

/* Sweep parameters: total range in kHz and step in kHz */
#define FREQUENCY_SWEEP_RANGE_KHZ 300
#define FREQUENCY_SWEEP_STEP_KHZ  3

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
 * MenuCalibrateXtal
 *
 * Interactive RF calibration routine.
 *
 * Steps:
 *  - Prompt the user and wait for confirmation to start tuning
 *  - Disable automatic frequency tracking and set a narrow bandwidth for the sweep
 *  - Sweep the receiver frequency from center - range/2 to center + range/2
 *    using small steps and count received master request messages at each step
 *  - Track the first and last frequency where master requests are detected
 *  - If a valid working range is detected, compute center frequency and range,
 *    display results and offer to save the computed offset to EEPROM
 *  - Restore RF driver settings (bandwidth, frequency offset) on exit
 *
 * The function performs blocking console IO and returns when tuning completes
 * or when the user presses ESC.
 */
void MenuCalibrateXtal()
{
    bool          exitTuneLoop;
    bool          updateFreq;
    uint32_t      lastMessageTime      = millis();
    float         currentFreq_MHz      = MICRONET_RF_CENTER_FREQUENCY_MHZ - (FREQUENCY_SWEEP_RANGE_KHZ / 2000.0f);
    float         firstWorkingFreq_MHz = 100000;
    float         lastWorkingFreq_MHz  = 0;
    float         range_kHz, centerFrequency_MHz;
    char          c;
    MicronetCodec micronetCodec;

    CONSOLE.println("");
    CONSOLE.println("To tune RF frequency, you must start your Micronet network and");
    CONSOLE.println("put MicronetToNMEA HW close to your Micronet main display (less than one meter).");
    CONSOLE.println("You must not move any of the devices during the tuning phase.");
    CONSOLE.println("Tuning phase will last about two minutes.");
    CONSOLE.println("Press any key when you are ready to start.");

    while (CONSOLE.available() == 0)
    {
        yield();
    }

    CONSOLE.println("");
    CONSOLE.println("Starting Frequency tuning");
    CONSOLE.println("Press ESC key at any time to stop tuning and come back to menu.");
    CONSOLE.println("");

    gRfReceiver.DisableFrequencyTracking();
    gRfReceiver.SetFrequencyOffset(0);
    gRfReceiver.SetBandwidth(RfDriver::RF_BANDWIDTH_LOW);
    gRfReceiver.SetFrequency(currentFreq_MHz);

    updateFreq   = false;
    exitTuneLoop = false;

    gRxMessageFifo.ResetFifo();
    do
    {
        MicronetMessage_t *rxMessage;
        if ((rxMessage = gRxMessageFifo.Peek()) != nullptr)
        {
            if (micronetCodec.GetMessageId(rxMessage) == MICRONET_MESSAGE_ID_MASTER_REQUEST)
            {
                lastMessageTime = millis();
                CONSOLE.print("*");
                updateFreq = true;
                if (currentFreq_MHz < firstWorkingFreq_MHz)
                    firstWorkingFreq_MHz = currentFreq_MHz;
                if (currentFreq_MHz > lastWorkingFreq_MHz)
                    lastWorkingFreq_MHz = currentFreq_MHz;
            }
            gRxMessageFifo.DeleteMessage();
        }

        if (millis() - lastMessageTime > 1250)
        {
            lastMessageTime = millis();
            CONSOLE.print(".");
            updateFreq = true;
        }

        if (updateFreq)
        {
            lastMessageTime = millis();
            updateFreq      = false;
            if (currentFreq_MHz < MICRONET_RF_CENTER_FREQUENCY_MHZ + (FREQUENCY_SWEEP_RANGE_KHZ / 2000.0f))
            {
                currentFreq_MHz += (FREQUENCY_SWEEP_STEP_KHZ / 1000.0f);
                gRfReceiver.SetFrequency(currentFreq_MHz);
            }
            else
            {
                centerFrequency_MHz = ((lastWorkingFreq_MHz + firstWorkingFreq_MHz) / 2);
                range_kHz           = (lastWorkingFreq_MHz - firstWorkingFreq_MHz) * 1000;
                if ((range_kHz > 0) && (range_kHz < FREQUENCY_SWEEP_RANGE_KHZ))
                {
                    CONSOLE.println("");
                    CONSOLE.print("Frequency = ");
                    CONSOLE.print(centerFrequency_MHz * 1000);
                    CONSOLE.println("kHz");
                    CONSOLE.print("Range = ");
                    CONSOLE.print(range_kHz);
                    CONSOLE.println("kHz");
                    CONSOLE.print("Deviation to real frequency = ");
                    CONSOLE.print((centerFrequency_MHz - MICRONET_RF_CENTER_FREQUENCY_MHZ) * 1000);
                    CONSOLE.print("kHz (");
                    CONSOLE.print((int)(1000000.0 * (centerFrequency_MHz - MICRONET_RF_CENTER_FREQUENCY_MHZ) / MICRONET_RF_CENTER_FREQUENCY_MHZ));
                    CONSOLE.println(" ppm)");

                    CONSOLE.println("Do you want to save the new RF calibration values (y/n) ?");
                    while (CONSOLE.available() == 0)
                        ;
                    c = CONSOLE.read();
                    if ((c == 'y') || (c == 'Y'))
                    {
                        gConfiguration.eeprom.rfFrequencyOffset_MHz = (centerFrequency_MHz - MICRONET_RF_CENTER_FREQUENCY_MHZ);
                        gConfiguration.SaveToEeprom();
                        CONSOLE.println("Configuration saved");
                    }
                    else
                    {
                        CONSOLE.println("Configuration discarded");
                    }
                }

                exitTuneLoop = true;
            }
        }

        while (CONSOLE.available() > 0)
        {
            if (CONSOLE.read() == 0x1b)
            {
                CONSOLE.println("\r\nESC key pressed, stopping frequency tuning.");
                exitTuneLoop = true;
            }
        }

        yield();
    } while (!exitTuneLoop);

    gRfReceiver.SetBandwidth(RfDriver::RF_BANDWIDTH_HIGH);
    gRfReceiver.SetFrequencyOffset(gConfiguration.eeprom.rfFrequencyOffset_MHz);
    gRfReceiver.SetFrequency(MICRONET_RF_CENTER_FREQUENCY_MHZ);
}

