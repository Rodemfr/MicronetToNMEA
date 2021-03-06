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

#include "BoardConfig.h"
#include "MicronetMessageFifo.h"
#include "MenuManager.h"
#include "Configuration.h"
#include "NmeaEncoder.h"
#include "Globals.h"
#include "MicronetCodec.h"
#include "NmeaDecoder.h"
#include "NavCompass.h"

#include <Arduino.h>
#include <SPI.h>
#include <wiring.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

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
void PrintNetworkMap(NetworkMap_t *networkMap);
void MenuAbout();
void MenuScanNetworks();
void MenuAttachNetwork();
void MenuConvertToNmea();
void MenuScanAllMicronetTraffic();
void MenuCalibrateMagnetoMeter();
void SaveCalibration();
void LoadCalibration();

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
{ "Scan all surrounding Micronet traffic", MenuScanAllMicronetTraffic },
{ "Calibrate magnetometer", MenuCalibrateMagnetoMeter },
{ nullptr, nullptr } };

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

void setup()
{
	// Load configuration from EEPROM
	gConfiguration.LoadFromEeprom();
	LoadCalibration();

	// Init USB serial link
	USB_CONSOLE.begin(USB_BAUDRATE);

	// Init GNSS NMEA serial link
	GNSS_SERIAL.setRX(GNSS_RX_PIN);
	GNSS_SERIAL.setTX(GNSS_TX_PIN);
	GNSS_SERIAL.begin(GNSS_BAUDRATE);

	// Init wired serial link
	WIRED_SERIAL.setRX(WIRED_RX_PIN);
	WIRED_SERIAL.setTX(WIRED_TX_PIN);
	WIRED_SERIAL.begin(WIRED_BAUDRATE);

	// Let time for serial drivers to set-up
	delay (250);

	// Setup main menu
	gMenuManager.SetMenu(mainMenu);

	// Set SPI pin configuration
	SPI.setMOSI(MOSI_PIN);
	SPI.setMISO(MISO_PIN);
	SPI.setSCK(SCK_PIN);
	SPI.setCS(CS0_PIN);
	SPI.begin();

	CONSOLE.print("Initializing CC1101 ... ");
	// Check connection to CC1101
	if (!gRfReceiver.Init(GDO0_PIN, &gRxMessageFifo))
	{
		CONSOLE.println("Failed");
		CONSOLE.println("Aborting execution : Verify connection to CC1101 board");
		CONSOLE.println("Halted");

		while (1)
		{
			digitalWrite(LED_PIN, HIGH);
			delay(500);
			digitalWrite(LED_PIN, LOW);
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

	// Attach callback to GDO0 pin
	// According to CC1101 configuration this callback will be executed when CC1101 will have detected Micronet's sync word
	attachInterrupt(digitalPinToInterrupt(GDO0_PIN), RfIsr, RISING);

	// Start listening
	gRfReceiver.RestartReception();

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

void GNSS_CALLBACK()
{
	// This callback is called each time we received data from the NMEA GNSS
	while (GNSS_SERIAL.available() > 0)
	{
		// Send the data to the decoder. The decoder does not actually decode the NMEA stream, it just stores it
		// to repeat it later to the console output.
		gGnssDecoder.PushChar(GNSS_SERIAL.read(), &gNavData);
	}
}

void RfIsr()
{
	gRfReceiver.GDO0Callback();
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

void PrintNetworkMap(NetworkMap_t *networkMap)
{
	CONSOLE.print("Network ID : 0x");
	PrintInt(networkMap->networkId);
	CONSOLE.println("");

	CONSOLE.print("Nb Devices : ");
	CONSOLE.println(networkMap->nbDevices);

	for (uint32_t i = 0; i < networkMap->nbSlots; i++)
	{
		CONSOLE.print(i);
		CONSOLE.print(" : 0x");
		PrintInt(networkMap->syncSlot[i].deviceId);
		CONSOLE.print(" ");
		CONSOLE.print(networkMap->syncSlot[i].payloadBytes);
		CONSOLE.print(" ");
		CONSOLE.print(networkMap->syncSlot[i].start_us);
		CONSOLE.print(" ");
		CONSOLE.println(networkMap->syncSlot[i].length_us);
	}
}

void MenuAbout()
{
	CONSOLE.println("MicronetToNMEA, Version 0.4");

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
	CONSOLE.println("Provides the following NMEA sentences :");
	CONSOLE.println(" - INDPT (Depth below transducer. T121 with depth sounder required)");
	CONSOLE.println(" - INMWV (Apparent wind. T120 required)");
	CONSOLE.println(" - INMWV (True wind. T120 and T121 with Speedo/Temp sensor required)");
	CONSOLE.println(" - INMTW (Water temperature. T121 with Speedo/Temp sensor required)");
	CONSOLE.println(" - INVHW (Speed on water. T121 with Speedo/Temp sensor required)");
	CONSOLE.println(" - INVLW (Distance log T121 with Speedo/Temp sensor required)");
}

void MenuScanNetworks()
{
	MicronetMessage_t *message;
	uint32_t nidArray[MAX_SCANNED_NETWORKS];
	int16_t rssiArray[MAX_SCANNED_NETWORKS];

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
			if (gMicronetCodec.VerifyHeaderCrc(message))
			{
				uint32_t nid = gMicronetCodec.GetNetworkId(message);
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
	char nmeaSentence[256];
	MicronetMessage_t *rxMessage;
	MicronetMessage_t txMessage;
	TxSlotDesc_t txSlot;
	uint8_t payloadLength;
	uint32_t lastHeadingTime = millis();
	float heading;

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

	gRxMessageFifo.ResetFifo();

	do
	{
		if ((rxMessage = gRxMessageFifo.Peek()) != nullptr)
		{
			if ((gMicronetCodec.GetNetworkId(rxMessage) == gConfiguration.networkId)
					&& (gMicronetCodec.VerifyHeaderCrc(rxMessage)))
			{
				if (gMicronetCodec.GetMessageId(rxMessage) == MICRONET_MESSAGE_ID_REQUEST_DATA)
				{
					txSlot = gMicronetCodec.GetSyncTransmissionSlot(rxMessage, gConfiguration.deviceId);
					if (txSlot.start_us != 0)
					{
						payloadLength = gMicronetCodec.EncodeDataMessage(&txMessage, gConfiguration.networkId,
								gConfiguration.deviceId, &gNavData);
						if (txSlot.payloadBytes < payloadLength)
						{
							txSlot = gMicronetCodec.GetAsyncTransmissionSlot(rxMessage);
							gMicronetCodec.EncodeSlotUpdateMessage(&txMessage, gConfiguration.networkId, gConfiguration.deviceId,
									payloadLength);
						}
					}
					else
					{
						txSlot = gMicronetCodec.GetAsyncTransmissionSlot(rxMessage);
						gMicronetCodec.EncodeSlotRequestMessage(&txMessage, gConfiguration.networkId, gConfiguration.deviceId,
								52);
					}
					gRfReceiver.TransmitMessage(&txMessage, txSlot.start_us);

					if (gNmeaEncoder.EncodeMWV_R(&gNavData, nmeaSentence))
						NMEA_OUT.print(nmeaSentence);
					if (gNmeaEncoder.EncodeMWV_T(&gNavData, nmeaSentence))
						NMEA_OUT.print(nmeaSentence);
					if (gNmeaEncoder.EncodeDPT(&gNavData, nmeaSentence))
						NMEA_OUT.print(nmeaSentence);
					if (gNmeaEncoder.EncodeMTW(&gNavData, nmeaSentence))
						NMEA_OUT.print(nmeaSentence);
					if (gNmeaEncoder.EncodeVLW(&gNavData, nmeaSentence))
						NMEA_OUT.print(nmeaSentence);
					if (gNmeaEncoder.EncodeVHW(&gNavData, nmeaSentence))
						NMEA_OUT.print(nmeaSentence);
					if (gNavData.calibrationUpdated)
					{
						gNavData.calibrationUpdated = false;
						SaveCalibration();
					}
				} else {
					gMicronetCodec.DecodeDataMessage(rxMessage, &gNavData);
				}
			}
			gRxMessageFifo.DeleteMessage();
		}

		int nbGnssSentences = gGnssDecoder.GetNbSentences();
		for (int i = 0; i < nbGnssSentences; i++)
		{
			NMEA_OUT.println(gGnssDecoder.GetSentence(i));
		}
		gGnssDecoder.resetSentences();

		char c;
		while (NMEA_IN.available() > 0)
		{
			c = NMEA_IN.read();
			if ((CONSOLE == NMEA_IN) && (c == 0x1b))
			{
				CONSOLE.println("ESC key pressed, stopping conversion.");
				exitNmeaLoop = true;
			}
			gNavDecoder.PushChar(c, &gNavData);
		}
		gNavDecoder.resetSentences();

		// Only execute mangetic heading code if navigation compass is available
		if (gConfiguration.navCompassAvailable == true)
		{
			// Handle magnetic compass
			// Only request new reading if previous is at least 100ms old
			if ((millis() - lastHeadingTime) > 100)
			{
				lastHeadingTime = millis();
				heading = gNavCompass.GetHeading();
				gNavData.hdg_deg.value = heading;
				gNavData.hdg_deg.valid = true;
				gNavData.hdg_deg.timeStamp = lastHeadingTime;
			}
		}

		if (CONSOLE != NMEA_IN)
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

		gNavData.UpdateValidity();

		yield();
	} while (!exitNmeaLoop);
}

void MenuScanAllMicronetTraffic()
{
	bool exitSniffLoop = false;
	uint32_t lastMasterRequest_us = 0;

	CONSOLE.println("Starting Micronet traffic scanning.");
	CONSOLE.println("Press ESC key at any time to stop scanning and come back to menu.");
	CONSOLE.println("");

	gRxMessageFifo.ResetFifo();

	MicronetMessage_t *message;
	do
	{
		if ((message = gRxMessageFifo.Peek()) != nullptr)
		{
			if (gMicronetCodec.VerifyHeaderCrc(message))
			{
				if (message->data[MICRONET_MI_OFFSET] == MICRONET_MESSAGE_ID_REQUEST_DATA)
				{
					CONSOLE.println("");
					lastMasterRequest_us = message->endTime_us;
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
	uint32_t currentTime;
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
		currentTime = millis();
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
}

void SaveCalibration()
{
	gConfiguration.waterSpeedFactor_per = gNavData.waterSpeedFactor_per;
	gConfiguration.waterTemperatureOffset_C = gNavData.waterTemperatureOffset_degc;
	gConfiguration.depthOffset_m = gNavData.depthOffset_m;
	gConfiguration.windSpeedFactor_per = gNavData.windSpeedFactor_per;
	gConfiguration.windDirectionOffset_deg = gNavData.windDirectionOffset_deg;
	gConfiguration.headingOffset_deg = gNavData.headingOffset_deg;
	gConfiguration.magneticVariation_deg = gNavData.magneticVariation_deg;
	gConfiguration.windShift = gNavData.windShift_min;

	gConfiguration.SaveToEeprom();
}

void LoadCalibration()
{
	gNavData.waterSpeedFactor_per = gConfiguration.waterSpeedFactor_per;
	gNavData.waterTemperatureOffset_degc = gConfiguration.waterTemperatureOffset_C;
	gNavData.depthOffset_m = gConfiguration.depthOffset_m;
	gNavData.windSpeedFactor_per = gConfiguration.windSpeedFactor_per;
	gNavData.windDirectionOffset_deg = gConfiguration.windDirectionOffset_deg;
	gNavData.headingOffset_deg = gConfiguration.headingOffset_deg;
	gNavData.magneticVariation_deg = gConfiguration.magneticVariation_deg;
	gNavData.windShift_min = gConfiguration.windShift;
}
