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
#include "Version.h"
#include "Micronet.h"
#include "MicronetMessageFifo.h"
#include "MenuManager.h"
#include "Configuration.h"
#include "Globals.h"
#include "MicronetCodec.h"
#include "NavCompass.h"

#include <SPI.h>
#include <Wire.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define MAX_SCANNED_NETWORKS  5
#define WAIT_TIME_US(t) {while (micros() < t);}

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

void RfIsr();
void PrintByte(uint8_t data);
void PrintInt(uint32_t data);
void PrintRawMessage(MicronetMessage_t *message, uint32_t lastMasterRequest_us);
void PrintNetworkMap(MicronetCodec::NetworkMap *networkMap);
void PrintMessageFifo(MicronetMessageFifo &messageFifo);
void MenuAbout();
void MenuScanNetworks();
void MenuAttachNetwork();
void MenuConvertToNmea();
void MenuScanAllMicronetTraffic();
void MenuCalibrateMagnetoMeter();
void MenuCalibrateRfFrequency();
void MenuTestRfQuality();
void SaveCalibration(MicronetCodec &micronetCodec);
void LoadCalibration(MicronetCodec &micronetCodec);
void ConfigureSlaveDevice(MicronetSlaveDevice &micronetDevice);

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

bool firstLoop;

MenuEntry_t mainMenu[] =
{
{ "MicronetToNMEA", nullptr },
{ "General info on MicronetToNMEA", MenuAbout },
{ "Scan Micronet networks", MenuScanNetworks },
{ "Attach converter to a network", MenuAttachNetwork },
{ "Start NMEA conversion", MenuConvertToNmea },
{ "Scan surrounding Micronet traffic", MenuScanAllMicronetTraffic },
{ "Calibrate RF frequency", MenuCalibrateRfFrequency },
{ "Calibrate magnetometer", MenuCalibrateMagnetoMeter },
{ "Test RF quality", MenuTestRfQuality },
{ nullptr, nullptr } };

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

void setup()
{
	// Load configuration from EEPROM
	gConfiguration.LoadFromEeprom();

	// Init USB serial link
	USB_NMEA.begin(USB_BAUDRATE);

	// Init GNSS NMEA serial link
	GNSS_SERIAL.setRX(GNSS_RX_PIN);
	GNSS_SERIAL.setTX(GNSS_TX_PIN);
	GNSS_SERIAL.begin(GNSS_BAUDRATE);

	// Init wired serial link
	WIRED_NMEA.setRX(WIRED_RX_PIN);
	WIRED_NMEA.setTX(WIRED_TX_PIN);
	WIRED_NMEA.begin(WIRED_BAUDRATE);

	// Let time for serial drivers to set-up
	delay(250);

#if (GNSS_UBLOXM8N == 1)
	CONSOLE.println("Configuring UBlox M8N GNSS");
	gM8nDriver.Start(M8N_GGA_ENABLE | M8N_VTG_ENABLE | M8N_RMC_ENABLE);
#endif

	// Setup main menu
	gMenuManager.SetMenu(mainMenu);

	CONSOLE.print("Initializing CC1101 ... ");
	// Check connection to CC1101
	if (!gRfReceiver.Init(&gRxMessageFifo, gConfiguration.rfFrequencyOffset_MHz))
	{
		CONSOLE.println("Failed");
		CONSOLE.println("Aborting execution : Verify connection to CC1101 board");
		CONSOLE.println("Halted");

		while (1)
		{
			digitalWrite(LED_BUILTIN, HIGH);
			delay(500);
			digitalWrite(LED_BUILTIN, LOW);
			delay(500);
		}
	}
	CONSOLE.println("OK");

	CONSOLE.print("Initializing navigation compass ... ");
	if (!gNavCompass.Init())
	{
		CONSOLE.println("NOT DETECTED");
		gConfiguration.navCompassAvailable = false;
	}
	else
	{
		CONSOLE.print(gNavCompass.GetDeviceName().c_str());
		CONSOLE.println(" Found");
		gConfiguration.navCompassAvailable = true;
	}

	// Start listening
	gRfReceiver.RestartReception();

	// Attach callback to GDO0 pin
	// According to CC1101 configuration this callback will be executed when CC1101 will have detected Micronet's sync word
#if defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
	attachInterrupt(digitalPinToInterrupt(GDO0_PIN), RfIsr, RISING);
#else
	attachInterrupt(digitalPinToInterrupt(GDO0_PIN), RfIsr, HIGH);
#endif

	// Display serial menu
	gMenuManager.PrintMenu();

	// For the main loop to know when it is executing for the first time
	firstLoop = true;
}

void loop()
{
	// If this is the first loop, we verify if we are already attached to a Micronet network. if yes,
	// We directly jump to NMEA conversion mode.
	if ((firstLoop) && (gConfiguration.networkId != 0))
	{
		MenuConvertToNmea();
		gMenuManager.PrintMenu();
	}

	// Process console input
	while (CONSOLE.available() > 0)
	{
		gMenuManager.PushChar(CONSOLE.read());
	}

	firstLoop = false;
}

void RfIsr()
{
	gRfReceiver.RfIsr();
}

void PrintByte(uint8_t data)
{
	if (data < 16)
	{
		CONSOLE.print("0");
	}
	CONSOLE.print(data, HEX);
}

void PrintInt(uint32_t data)
{
	PrintByte((data >> 24) & 0x0ff);
	PrintByte((data >> 16) & 0x0ff);
	PrintByte((data >> 8) & 0x0ff);
	PrintByte(data & 0x0ff);
}

void PrintRawMessage(MicronetMessage_t *message, uint32_t lastMasterRequest_us)
{
	if (message->len < MICRONET_PAYLOAD_OFFSET)
	{
		CONSOLE.print("Invalid message (");
		CONSOLE.print((int) message->rssi);
		CONSOLE.print(", ");
		CONSOLE.print((int) (message->startTime_us - lastMasterRequest_us));
		CONSOLE.println(")");
	}

	for (int j = 0; j < 4; j++)
	{
		PrintByte(message->data[j]);
	}
	CONSOLE.print(" ");

	for (int j = 4; j < 8; j++)
	{
		PrintByte(message->data[j]);
	}
	CONSOLE.print(" ");

	for (int j = 8; j < 14; j++)
	{
		PrintByte(message->data[j]);
		CONSOLE.print(" ");
	}

	CONSOLE.print(" -- ");

	for (int j = 14; j < message->len; j++)
	{
		PrintByte(message->data[j]);
		CONSOLE.print(" ");
	}

	CONSOLE.print(" (");
	CONSOLE.print((int) message->rssi);
	CONSOLE.print(", ");
	CONSOLE.print((int) (message->startTime_us - lastMasterRequest_us));
	CONSOLE.print(")");

	CONSOLE.println();
}

void PrintNetworkMap(MicronetCodec::NetworkMap *networkMap)
{
	CONSOLE.print("Network ID : 0x");
	PrintInt(networkMap->networkId);
	CONSOLE.println("");

	CONSOLE.print("Nb Devices : ");
	CONSOLE.println(networkMap->nbSyncSlots);
	CONSOLE.print("Master : ");
	CONSOLE.print(" : 0x");
	PrintInt(networkMap->masterDevice);
	CONSOLE.println("");

	for (uint32_t i = 0; i < networkMap->nbSyncSlots; i++)
	{
		CONSOLE.print("S");
		CONSOLE.print(i);
		CONSOLE.print(" : 0x");
		PrintInt(networkMap->syncSlot[i].deviceId);
		CONSOLE.print(" ");
		if (networkMap->syncSlot[i].start_us > 0)
		{
			CONSOLE.print(networkMap->syncSlot[i].payloadBytes);
			CONSOLE.print(" ");
			CONSOLE.print(networkMap->syncSlot[i].start_us - networkMap->firstSlot);
			CONSOLE.print(" ");
			CONSOLE.println(networkMap->syncSlot[i].length_us);
		}
		else
		{
			CONSOLE.println("-");
		}
	}

	CONSOLE.print("Async : ");
	CONSOLE.print(" ");
	CONSOLE.print(networkMap->asyncSlot.payloadBytes);
	CONSOLE.print(" ");
	CONSOLE.print(networkMap->asyncSlot.start_us - networkMap->firstSlot);
	CONSOLE.print(" ");
	CONSOLE.println(networkMap->asyncSlot.length_us);

	for (uint32_t i = 0; i < networkMap->nbAckSlots; i++)
	{
		CONSOLE.print("A");
		CONSOLE.print(i);
		CONSOLE.print(" : 0x");
		PrintInt(networkMap->ackSlot[i].deviceId);
		CONSOLE.print(" ");
		CONSOLE.print(networkMap->ackSlot[i].payloadBytes);
		CONSOLE.print(" ");
		CONSOLE.print(networkMap->ackSlot[i].start_us - networkMap->firstSlot);
		CONSOLE.print(" ");
		CONSOLE.println(networkMap->ackSlot[i].length_us);
	}
	CONSOLE.println("");
}

void PrintMessageFifo(MicronetMessageFifo &messageFifo)
{
	if (messageFifo.GetNbMessages() > 0)
	{
		for (int i = 0; i < messageFifo.GetNbMessages(); i++)
		{
			MicronetMessage_t *message = messageFifo.Peek(i);
			CONSOLE.print("MSG ");
			CONSOLE.print(i);
			CONSOLE.print(" : ");
			CONSOLE.print(message->startTime_us);
			CONSOLE.print("/");
			CONSOLE.print(message->len);
			CONSOLE.print(" ");
			CONSOLE.println(message->action);
		}
		CONSOLE.println("");
	}
}

void MenuAbout()
{
	CONSOLE.print("MicronetToNMEA, Version ");
	CONSOLE.print(MNET2NMEA_SW_MAJOR_VERSION, DEC);
	CONSOLE.print(".");
	CONSOLE.println(MNET2NMEA_SW_MINOR_VERSION, DEC);

	CONSOLE.print("Device ID : ");
	CONSOLE.println(gConfiguration.deviceId, HEX);

	if (gConfiguration.networkId != 0)
	{
		CONSOLE.print("Attached to Micronet Network ");
		CONSOLE.println(gConfiguration.networkId, HEX);
	}
	else
	{
		CONSOLE.println("No Micronet Network attached");
	}

	CONSOLE.print("RF Frequency offset = ");
	CONSOLE.print(gConfiguration.rfFrequencyOffset_MHz * 1000);
	CONSOLE.print(" kHz (");
	CONSOLE.print((int) (1000000.0 * gConfiguration.rfFrequencyOffset_MHz / MICRONET_RF_CENTER_FREQUENCY_MHZ));
	CONSOLE.println(" ppm)");
	CONSOLE.print("Wind speed factor = ");
	CONSOLE.println(gConfiguration.windSpeedFactor_per);
	CONSOLE.print("Wind direction offset = ");
	CONSOLE.println((int) (gConfiguration.windDirectionOffset_deg));
	CONSOLE.print("Water speed factor = ");
	CONSOLE.println(gConfiguration.waterSpeedFactor_per);
	CONSOLE.print("Water temperature offset = ");
	CONSOLE.println((int) (gConfiguration.waterTemperatureOffset_C));
	if (gConfiguration.navCompassAvailable == false)
	{
		CONSOLE.println("No navigation compass detected, disabling magnetic heading.");
	}
	else
	{
		CONSOLE.print("Using ");
		CONSOLE.print(gNavCompass.GetDeviceName().c_str());
		CONSOLE.println(" for magnetic heading");
		CONSOLE.print("Magnetometer calibration : ");
		CONSOLE.print(gConfiguration.xMagOffset);
		CONSOLE.print(" ");
		CONSOLE.print(gConfiguration.yMagOffset);
		CONSOLE.print(" ");
		CONSOLE.println(gConfiguration.zMagOffset);
	}
}

void MenuScanNetworks()
{
	MicronetMessage_t *message;
	uint32_t nidArray[MAX_SCANNED_NETWORKS];
	int16_t rssiArray[MAX_SCANNED_NETWORKS];
	MicronetCodec micronetCodec;

	memset(nidArray, 0, sizeof(nidArray));
	memset(rssiArray, 0, sizeof(rssiArray));

	CONSOLE.print("Scanning Micronet networks for 5 seconds ... ");

	gRxMessageFifo.ResetFifo();
	unsigned long startTime = millis();
	do
	{
		if ((message = gRxMessageFifo.Peek()) != nullptr)
		{
			// Only consider messages with a valid CRC
			if (micronetCodec.VerifyHeaderCrc(message))
			{
				uint32_t nid = micronetCodec.GetNetworkId(message);
				int16_t rssi = message->rssi;
				// Store the network in the array by order of reception power
				for (int i = 0; i < MAX_SCANNED_NETWORKS; i++)
				{
					if (nidArray[i] == 0)
					{
						// New network
						nidArray[i] = nid;
						rssiArray[i] = rssi;
						break;
					}
					else if (nidArray[i] == nid)
					{
						// Already scanned network : update RSSI if stronger
						if (rssi > rssiArray[i])
						{
							rssiArray[i] = rssi;
						}
						break;
					}
					else
					{
						// New network to be inserted in the list : shift the list down
						if (rssi > rssiArray[i])
						{
							for (int j = (MAX_SCANNED_NETWORKS - 1); j > i; j++)
							{
								nidArray[j] = nidArray[j - 1];
								rssiArray[j] = rssiArray[j - 1];
							}
							nidArray[i] = nid;
							rssiArray[i] = rssi;
							break;
						}
					}
				}
			}
			gRxMessageFifo.DeleteMessage();
		}
	} while ((millis() - startTime) < 5000);

	CONSOLE.println("done");
	CONSOLE.println("");

	// Print result
	if (nidArray[0] != 0)
	{
		CONSOLE.println("List of scanned networks :");
		CONSOLE.println("");
		for (int i = 0; i < MAX_SCANNED_NETWORKS; i++)
		{
			if (nidArray[i] != 0)
			{
				CONSOLE.print("Network ");
				CONSOLE.print(i);
				CONSOLE.print(" - ");
				CONSOLE.print(nidArray[i], HEX);
				CONSOLE.print(" (");
				if (rssiArray[i] < 70)
					CONSOLE.print("very strong");
				else if (rssiArray[i] < 80)
					CONSOLE.print("strong");
				else if (rssiArray[i] < 90)
					CONSOLE.print("normal");
				else
					CONSOLE.print("low");
				CONSOLE.println(")");
			}
		}
	}
	else
	{
		CONSOLE.println("/!\\ No Micronet network found /!\\");
		CONSOLE.println("Check that your Micronet network is powered on.");
	}
}

void MenuAttachNetwork()
{
	char input[16], c;
	uint32_t charIndex = 0;

	CONSOLE.print("Enter Network ID to attach to : ");

	do
	{
		if (CONSOLE.available())
		{
			c = CONSOLE.read();
			if (c == 0x0d)
			{
				CONSOLE.println("");
				break;
			}
			else if ((c == 0x08) && (charIndex > 0))
			{
				charIndex--;
				CONSOLE.print(c);
				CONSOLE.print(" ");
				CONSOLE.print(c);
			}
			else if (charIndex < sizeof(input))
			{
				input[charIndex++] = c;
				CONSOLE.print(c);
			}
		};
	} while (1);

	bool invalidInput = false;
	uint32_t newNetworkId = 0;

	if (charIndex == 0)
	{
		invalidInput = true;
	}

	for (uint32_t i = 0; i < charIndex; i++)
	{
		c = input[i];
		if ((c >= '0') && (c <= '9'))
		{
			c -= '0';
		}
		else if ((c >= 'a') && (c <= 'f'))
		{
			c = c - 'a' + 10;
		}
		else if ((c >= 'A') && (c <= 'F'))
		{
			c = c - 'A' + 10;
		}
		else
		{
			invalidInput = true;
			break;
		}

		newNetworkId = (newNetworkId << 4) | c;
	}

	if (invalidInput)
	{
		CONSOLE.println("Invalid Network ID entered, ignoring input.");
	}
	else
	{
		gConfiguration.networkId = newNetworkId;
		CONSOLE.print("Now attached to NetworkID ");
		CONSOLE.println(newNetworkId, HEX);
		gConfiguration.SaveToEeprom();
	}
}

void MenuConvertToNmea()
{
	bool exitNmeaLoop = false;
	MicronetMessage_t *rxMessage;
	MicronetMessageFifo txMessageFifo;
	uint32_t lastHeadingTime = millis();
	MicronetCodec micronetCodec;
	DataBridge dataBridge(&micronetCodec);
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

		while (GNSS_SERIAL.available() > 0)
		{
			dataBridge.PushNmeaChar(GNSS_SERIAL.read(), LINK_NMEA_GNSS);
		}

		char c;
		while (NMEA_EXT.available() > 0)
		{
			c = NMEA_EXT.read();
			if (((void *)(&CONSOLE) == (void *)(&NMEA_EXT)) && (c == 0x1b))
			{
				CONSOLE.println("ESC key pressed, stopping conversion.");
				exitNmeaLoop = true;
			}
			dataBridge.PushNmeaChar(c, LINK_NMEA_EXT);
		}

		// Only execute magnetic heading code if navigation compass is available
		if (gConfiguration.navCompassAvailable == true)
		{
			// Handle magnetic compass
			// Only request new reading if previous is at least 100ms old
			if ((millis() - lastHeadingTime) > 100)
			{
				lastHeadingTime = millis();
				dataBridge.UpdateCompassData(gNavCompass.GetHeading());
			}
		}

		if ((void *)(&CONSOLE) != (void *)(&NMEA_EXT))
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
		asm (
				"wfi\n"
				);
#endif

	} while (!exitNmeaLoop);

	gRfReceiver.DisableFrequencyTracking();
}

void MenuScanAllMicronetTraffic()
{
	bool exitSniffLoop = false;
	uint32_t lastMasterRequest_us = 0;
	MicronetCodec::NetworkMap networkMap;
	MicronetCodec micronetCodec;

	CONSOLE.println("Starting Micronet traffic scanning.");
	CONSOLE.println("Press ESC key at any time to stop scanning and come back to menu.");
	CONSOLE.println("");

	gRxMessageFifo.ResetFifo();

	MicronetMessage_t *message;
	do
	{
		if ((message = gRxMessageFifo.Peek()) != nullptr)
		{
			if (micronetCodec.VerifyHeaderCrc(message))
			{
				if (message->data[MICRONET_MI_OFFSET] == MICRONET_MESSAGE_ID_MASTER_REQUEST)
				{
					CONSOLE.println("");
					lastMasterRequest_us = message->endTime_us;
					micronetCodec.GetNetworkMap(message, &networkMap);
					PrintNetworkMap(&networkMap);
				}
				PrintRawMessage(message, lastMasterRequest_us);
			}
			gRxMessageFifo.DeleteMessage();
		}

		while (CONSOLE.available() > 0)
		{
			if (CONSOLE.read() == 0x1b)
			{
				CONSOLE.println("ESC key pressed, stopping scan.");
				exitSniffLoop = true;
			}
		}
		yield();
	} while (!exitSniffLoop);
}

void MenuCalibrateMagnetoMeter()
{
	bool exitLoop = false;
	uint32_t pDisplayTime = 0;
	uint32_t pSampleTime = 0;
	float mx, my, mz;
	float xMin = 1000;
	float xMax = -1000;
	float yMin = 1000;
	float yMax = -1000;
	float zMin = 1000;
	float zMax = -1000;
	char c;

	if (gConfiguration.navCompassAvailable == false)
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
		gConfiguration.xMagOffset = (xMin + xMax) / 2;
		gConfiguration.yMagOffset = (yMin + yMax) / 2;
		gConfiguration.zMagOffset = (zMin + zMax) / 2;
		gConfiguration.SaveToEeprom();
		CONSOLE.println("Configuration saved");
	}
	else
	{
		CONSOLE.println("Configuration discarded");
	}
}

void MenuCalibrateRfFrequency()
{
#define FREQUENCY_SWEEP_RANGE_KHZ 300
#define FREQUENCY_SWEEP_STEP_KHZ 3

	bool exitTuneLoop;
	bool updateFreq;
	uint32_t lastMessageTime = millis();
	float currentFreq_MHz = MICRONET_RF_CENTER_FREQUENCY_MHZ - (FREQUENCY_SWEEP_RANGE_KHZ / 2000.0f);
	float firstWorkingFreq_MHz = 100000;
	float lastWorkingFreq_MHz = 0;
	float range_kHz, centerFrequency_MHz;
	char c;
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

	updateFreq = false;
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
			updateFreq = false;
			if (currentFreq_MHz < MICRONET_RF_CENTER_FREQUENCY_MHZ + (FREQUENCY_SWEEP_RANGE_KHZ / 2000.0f))
			{
				currentFreq_MHz += (FREQUENCY_SWEEP_STEP_KHZ / 1000.0f);
				gRfReceiver.SetFrequency(currentFreq_MHz);
			}
			else
			{
				centerFrequency_MHz = ((lastWorkingFreq_MHz + firstWorkingFreq_MHz) / 2);
				range_kHz = (lastWorkingFreq_MHz - firstWorkingFreq_MHz) * 1000;
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
					CONSOLE.print(
							(int) (1000000.0 * (centerFrequency_MHz - MICRONET_RF_CENTER_FREQUENCY_MHZ)
									/ MICRONET_RF_CENTER_FREQUENCY_MHZ));
					CONSOLE.println(" ppm)");

					CONSOLE.println("Do you want to save the new RF calibration values (y/n) ?");
					while (CONSOLE.available() == 0)
						;
					c = CONSOLE.read();
					if ((c == 'y') || (c == 'Y'))
					{
						gConfiguration.rfFrequencyOffset_MHz = (centerFrequency_MHz - MICRONET_RF_CENTER_FREQUENCY_MHZ);
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
	gRfReceiver.SetFrequencyOffset(gConfiguration.rfFrequencyOffset_MHz);
	gRfReceiver.SetFrequency(MICRONET_RF_CENTER_FREQUENCY_MHZ);
}

void MenuTestRfQuality()
{
	bool exitTestLoop = false;
	MicronetCodec::NetworkMap networkMap;
	float strength;
	TxSlotDesc_t txSlot;
	MicronetMessage_t txMessage;
	uint32_t receivedDid[MICRONET_MAX_DEVICES_PER_NETWORK];
	MicronetCodec micronetCodec;

	CONSOLE.println("Starting RF signal quality test.");
	CONSOLE.println("Press ESC key at any time to stop testing and come back to menu.");
	CONSOLE.println("");

	gRxMessageFifo.ResetFifo();

	do
	{
		MicronetMessage_t *message;

		if ((message = gRxMessageFifo.Peek()) != nullptr)
		{
			if (micronetCodec.VerifyHeaderCrc(message))
			{
				if (message->data[MICRONET_MI_OFFSET] == MICRONET_MESSAGE_ID_MASTER_REQUEST)
				{
					CONSOLE.println("");
					micronetCodec.GetNetworkMap(message, &networkMap);
					txSlot = micronetCodec.GetAsyncTransmissionSlot(&networkMap);
					micronetCodec.EncodePingMessage(&txMessage, 9, networkMap.networkId, gConfiguration.deviceId);
					txMessage.action = MICRONET_ACTION_RF_NO_ACTION;
					txMessage.startTime_us = txSlot.start_us;
					gRfReceiver.Transmit(&txMessage);
					memset(receivedDid, 0, sizeof(receivedDid));
				}

				bool alreadyReceived = false;
				for (int i = 0; i < MICRONET_MAX_DEVICES_PER_NETWORK; i++)
				{
					uint32_t did = micronetCodec.GetDeviceId(message);
					if (receivedDid[i] == did)
					{
						alreadyReceived = true;
						break;
					}
					else if (receivedDid[i] == 0)
					{
						receivedDid[i] = did;
						break;
					}
				}

				if (!alreadyReceived)
				{
					PrintInt(micronetCodec.GetDeviceId(message));
					CONSOLE.print(" Strength=");
					strength = micronetCodec.CalculateSignalFloatStrength(message);
					CONSOLE.print(strength);
					CONSOLE.print(" (");
					if (strength < 1.0)
					{
						CONSOLE.print("Very low");
					}
					else if (strength < 2.5)
					{
						CONSOLE.print("Low");
					}
					else if (strength < 5.0)
					{
						CONSOLE.print("Medium");
					}
					else if (strength < 7.0)
					{
						CONSOLE.print("Good");
					}
					else if (strength < 9.0)
					{
						CONSOLE.print("Very Good");
					}
					else
					{
						CONSOLE.print("Excellent");
					}

					CONSOLE.print(") ");
					switch (micronetCodec.GetDeviceType(message))
					{
					case MICRONET_DEVICE_TYPE_HULL_TRANSMITTER:
						CONSOLE.print("Hull Transmitter");
						break;
					case MICRONET_DEVICE_TYPE_WIND_TRANSDUCER:
						CONSOLE.print("Wind Transducer");
						break;
					case MICRONET_DEVICE_TYPE_NMEA_CONVERTER:
						CONSOLE.print("NMEA Converter");
						break;
					case MICRONET_DEVICE_TYPE_MAST_ROTATION:
						CONSOLE.print("Mast Rotation Sensor");
						break;
					case MICRONET_DEVICE_TYPE_MOB:
						CONSOLE.print("MOB");
						break;
					case MICRONET_DEVICE_TYPE_SDPOD:
						CONSOLE.print("SDPOD");
						break;
					case MICRONET_DEVICE_TYPE_DUAL_DISPLAY:
						CONSOLE.print("Dual Display");
						break;
					case MICRONET_DEVICE_TYPE_ANALOG_WIND_DISPLAY:
						CONSOLE.print("Analog Wind Display");
						break;
					case MICRONET_DEVICE_TYPE_DUAL_MAXI_DISPLAY:
						CONSOLE.print("Dual Maxi Display");
						break;
					case MICRONET_DEVICE_TYPE_REMOTE_DISPLAY:
						CONSOLE.print("Remote Display");
						break;
					default:
						CONSOLE.print("Unknown device");
						break;
					}
					if (networkMap.masterDevice == micronetCodec.GetDeviceId(message))
					{
						CONSOLE.print(", MASTER");
					}
					CONSOLE.println("");
				}
			}
			gRxMessageFifo.DeleteMessage();
		}

		while (CONSOLE.available() > 0)
		{
			if (CONSOLE.read() == 0x1b)
			{
				CONSOLE.println("ESC key pressed, stopping scan.");
				exitTestLoop = true;
			}
		}
		yield();
	} while (!exitTestLoop);
}

void SaveCalibration(MicronetCodec &micronetCodec)
{
	gConfiguration.waterSpeedFactor_per = micronetCodec.navData.waterSpeedFactor_per;
	gConfiguration.waterTemperatureOffset_C = micronetCodec.navData.waterTemperatureOffset_degc;
	gConfiguration.depthOffset_m = micronetCodec.navData.depthOffset_m;
	gConfiguration.windSpeedFactor_per = micronetCodec.navData.windSpeedFactor_per;
	gConfiguration.windDirectionOffset_deg = micronetCodec.navData.windDirectionOffset_deg;
	gConfiguration.headingOffset_deg = micronetCodec.navData.headingOffset_deg;
	gConfiguration.magneticVariation_deg = micronetCodec.navData.magneticVariation_deg;
	gConfiguration.windShift = micronetCodec.navData.windShift_min;

	gConfiguration.SaveToEeprom();
}

void LoadCalibration(MicronetCodec &micronetCodec)
{
	micronetCodec.navData.waterSpeedFactor_per = gConfiguration.waterSpeedFactor_per;
	micronetCodec.navData.waterTemperatureOffset_degc = gConfiguration.waterTemperatureOffset_C;
	micronetCodec.navData.depthOffset_m = gConfiguration.depthOffset_m;
	micronetCodec.navData.windSpeedFactor_per = gConfiguration.windSpeedFactor_per;
	micronetCodec.navData.windDirectionOffset_deg = gConfiguration.windDirectionOffset_deg;
	micronetCodec.navData.headingOffset_deg = gConfiguration.headingOffset_deg;
	micronetCodec.navData.magneticVariation_deg = gConfiguration.magneticVariation_deg;
	micronetCodec.navData.windShift_min = gConfiguration.windShift;
}

void ConfigureSlaveDevice(MicronetSlaveDevice &micronetDevice)
{
	// Configure Micronet's slave devices
	micronetDevice.SetNetworkId(gConfiguration.networkId);
	micronetDevice.SetDeviceId(gConfiguration.deviceId);
	micronetDevice.SetDataFields(
			DATA_FIELD_TIME | DATA_FIELD_SOGCOG | DATA_FIELD_DATE | DATA_FIELD_POSITION
					| DATA_FIELD_XTE | DATA_FIELD_DTW | DATA_FIELD_BTW | DATA_FIELD_VMGWP | DATA_FIELD_NODE_INFO);

	if (DEPTH_SOURCE_LINK != LINK_MICRONET)
	{
		micronetDevice.AddDataFields(DATA_FIELD_HDG);
	}

	if (DEPTH_SOURCE_LINK != LINK_MICRONET)
	{
		micronetDevice.AddDataFields(DATA_FIELD_DPT);
	}

	if (SPEED_SOURCE_LINK != LINK_MICRONET)
	{
		micronetDevice.AddDataFields(DATA_FIELD_SPD);
	}

	if ((MICRONET_WIND_REPEATER == 1) || (WIND_SOURCE_LINK != LINK_MICRONET))
	{
		micronetDevice.AddDataFields(DATA_FIELD_AWS | DATA_FIELD_AWA);
	}
}
