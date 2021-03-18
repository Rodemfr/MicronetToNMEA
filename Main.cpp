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

#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include "MicronetDecoder.h"
#include "MicronetMessageFifo.h"
#include "MenuManager.h"
#include "Configuration.h"
#include "NmeaEncoder.h"

#include <Arduino.h>
#include <SPI.h>
#include <wiring.h>
#include <iostream>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define CS0_PIN              10
#define MOSI_PIN             11
#define MISO_PIN             12
#define SCK_PIN              14
#define GDO0_PIN             24
#define GDO2_PIN             25
#define LED_PIN              LED_BUILTIN
#define MAX_SCANNED_NETWORKS 5

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

void RfReceiverIsr();
void RfFlushAndRestartRx();
void PrintRawMessage(MicronetMessage_t *message);
void PrintDecoderData(MicronetData_t *micronetData);
void MenuAbout();
void MenuScanNetworks();
void MenuAttachNetwork();
void MenuConvertToNmea();
void MenuScanAllMicronetTraffic();
void UpdateAndSaveCalibration();

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

ELECHOUSE_CC1101 gRfReceiver;         // CC1101 Driver object
MenuManager gMenuManager;         // Menu manager object
MicronetMessageFifo gMessageFifo; // Micronet message fifo store, used for communication between CC1101 ISR and main loop code
MicronetDecoder gMicronetDecoder; // Micronet message decoder
Configuration gConfiguration;
NmeaEncoder gNmeaEncoder;
bool firstLoop;

MenuEntry_t mainMenu[] =
{
{ "MicronetToNMEA", nullptr },
{ "General info on MicronetToNMEA", MenuAbout },
{ "Scan Micronet networks", MenuScanNetworks },
{ "Attach converter to a network", MenuAttachNetwork },
{ "Start NMEA conversion", MenuConvertToNmea },
{ "Scan all surrounding Micronet traffic", MenuScanAllMicronetTraffic },
{ nullptr, nullptr } };

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

void setup()
{
	// Load configuration from EEPROM
	gConfiguration.LoadFromEeprom();

	Serial.begin(gConfiguration.serialSpeed);

	// Setup serial menu
	gMenuManager.SetMenu(mainMenu);

	// Set SPI pin configuration
	SPI.setMOSI(MOSI_PIN);
	SPI.setMISO(MISO_PIN);
	SPI.setSCK(SCK_PIN);
	SPI.setCS(CS0_PIN);
	SPI.begin();

	// Check connection to CC1101
	if (!gRfReceiver.getCC1101())
	{
		Serial.println("Failed");
		Serial.println("Aborting execution : Verify connection to CC1101 board");
		Serial.println("Halted");

		while (1)
		{
			digitalWrite(LED_PIN, HIGH);
			delay(500);
			digitalWrite(LED_PIN, LOW);
			delay(500);
		}
	}

	// Configure CC1101 for listening Micronet devices
	gRfReceiver.Init(); // must be set to initialize the gReceiver!
	gRfReceiver.setGDO(GDO0_PIN, GDO2_PIN); // set lib internal gdo pins (gdo0,gdo2). Gdo2 not use for this example.
	gRfReceiver.setCCMode(1); // set config for internal transmission mode.
	gRfReceiver.setModulation(0); // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
	gRfReceiver.setMHZ(869.785); // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
	gRfReceiver.setDeviation(32); // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
	gRfReceiver.setChannel(0); // Set the Channelnumber from 0 to 255. Default is cahnnel 0.
	gRfReceiver.setChsp(199.95); // The channel spacing is multiplied by the channel number CHAN and added to the base frequency in kHz. Value from 25.39 to 405.45. Default is 199.95 kHz.
	gRfReceiver.setRxBW(250); // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
	gRfReceiver.setDRate(76.8); // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
	gRfReceiver.setPA(0); // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
	gRfReceiver.setSyncMode(2); // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
	gRfReceiver.setSyncWord(0x55, 0x99); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
	gRfReceiver.setAdrChk(0); // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
	gRfReceiver.setAddr(0); // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
	gRfReceiver.setWhiteData(0); // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
	gRfReceiver.setPktFormat(0); // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
	gRfReceiver.setLengthConfig(0); // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
	gRfReceiver.setPacketLength(60); // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
	gRfReceiver.setCrc(0); // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
	gRfReceiver.setCRC_AF(0); // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
	gRfReceiver.setDcFilterOff(0); // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity).
	gRfReceiver.setManchester(0); // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
	gRfReceiver.setFEC(0); // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
	gRfReceiver.setPQT(4); // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
	gRfReceiver.setAppendStatus(0); // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.

	// Attach callback to GDO0 pin
	// According to CC1101 configuration this callback will be executed when CC1101 will have detected Micronet's sync word
	attachInterrupt(digitalPinToInterrupt(GDO0_PIN), RfReceiverIsr, RISING);

	// Start listening
	gRfReceiver.SetRx();

	// Display serial menu
	gMenuManager.PrintMenu();

	// For the main loop to know when it is executing for the first time
	firstLoop = true;
}

void loop()
{
//	if ((firstLoop) && (gConfiguration.attachedNetworkId != 0))
//	{
//		MenuConvertToNmea();
//		gMenuManager.PrintMenu();
//	}

	// Process console input
	while (Serial.available() > 0)
	{
		gMenuManager.PushChar(Serial.read());
	}

	firstLoop = false;
}

void RfReceiverIsr()
{
	MicronetMessage_t message;
	int nbBytes;
	int dataOffset;
	bool newLengthFound = false;

	dataOffset = 0;
	// When we reach this point, we know that a packet is under reception by CC1101. We will not wait the end of this reception and will
	// begin collecting bytes right now. This way we will be able to instruct CC1101 to change packet size on the fly as soon as we will
	// have identified the length field
	do
	{
		// How many bytes are already in the FIFO ?
		nbBytes = gRfReceiver.SpiReadStatus(CC1101_RXBYTES);
		// Check for FIFO overflow
		if (nbBytes & 0x80)
		{
			// Yes : ignore current packet and restart CC1101 reception for the next packet
			RfFlushAndRestartRx();
			return;
		}
		// Are there new bytes in the FIFO ?
		if (nbBytes > 0)
		{
			// Yes : read them
			//gRfReceiver.ReadRxFifo(message.data + dataOffset, nbBytes);
			gRfReceiver.SpiReadBurstReg(CC1101_RXFIFO, message.data + dataOffset, nbBytes);
			dataOffset += nbBytes;
			// Check if we have reached the packet length field
			if ((!newLengthFound) && (dataOffset >= (MICRONET_LEN_OFFSET_1 + 2)))
			{
				newLengthFound = true;
				// Yes : check that this is a valid length
				if ((message.data[MICRONET_LEN_OFFSET_1] == message.data[MICRONET_LEN_OFFSET_2])
						&& (message.data[MICRONET_LEN_OFFSET_1] < MICRONET_MAX_MESSAGE_LENGTH - 3)
						&& ((message.data[MICRONET_LEN_OFFSET_1] + 2) >= MICRONET_PAYLOAD_OFFSET))
				{
					// Update CC1101's packet length register
					gRfReceiver.setPacketLength(message.data[MICRONET_LEN_OFFSET_1] + 2);
					// If CC1101 has already passed the number of bytes requested in the LEN field, setting packet length
					// to a value below the number of byte already counted by CC1101 will make GDO0 not to be deasserted,
					// so we have to add an extra check to leave the loop
					if (dataOffset >= message.data[MICRONET_LEN_OFFSET_1] + 2)
					{
						break;
					}
				}
				else
				{
					// The packet length is not valid : ignore current packet and restart CC1101 reception for the next packet
					RfFlushAndRestartRx();
					return;
				}
			}
		}
		// Continue reading as long as CC1101 doesn't tell us that the packet reception is finished
	} while (digitalRead(GDO0_PIN));

	// Collect remaining bytes that could have been missed in the last loop
	nbBytes = gRfReceiver.SpiReadStatus(CC1101_RXBYTES);
	if (nbBytes > 0)
	{
		gRfReceiver.SpiReadBurstReg(CC1101_RXFIFO, message.data + dataOffset, nbBytes);
		dataOffset += nbBytes;
	}
	// Restart CC1101 reception as soon as possible not to miss the next packet
	RfFlushAndRestartRx();
	// Don't consider the last two status bytes added by CC1101 in the packet length
	message.len = message.data[MICRONET_LEN_OFFSET_1] + 2;
	// Get RSSI for this message
	message.rssi = gRfReceiver.getRssi();

	// Add message to the store
	gMessageFifo.Push(message);
}

void RfFlushAndRestartRx()
{
	gRfReceiver.setSidle();
	gRfReceiver.SpiStrobe(CC1101_SFRX);
	gRfReceiver.SetRx();
}

void PrintRawMessage(MicronetMessage_t *message)
{
	for (int j = 0; j < message->len; j++)
	{
		if (message->data[j] < 16)
		{
			Serial.print("0");
		}
		Serial.print(message->data[j], HEX);
		Serial.print(" ");
	}
	Serial.print(" (");
	Serial.print((int) message->len);
	Serial.print(",");
	Serial.print((int) message->rssi);
	Serial.print(")");

	Serial.println();
}

void PrintDecoderData(MicronetData_t *micronetData)
{
	if (micronetData->awa.valid)
	{
		Serial.print("AWA ");
		Serial.print(micronetData->awa.value);
		Serial.print("deg | ");
	}
	if (micronetData->aws.valid)
	{
		Serial.print("AWS ");
		Serial.print(micronetData->aws.value);
		Serial.print("kt | ");
	}
	if (micronetData->stw.valid)
	{
		Serial.print("STW ");
		Serial.print(micronetData->stw.value);
		Serial.print("kt | ");
	}
	if (micronetData->dpt.valid)
	{
		Serial.print("DPT ");
		Serial.print(micronetData->dpt.value);
		Serial.print("M | ");
	}
	if (micronetData->vcc.valid)
	{
		Serial.print("VCC ");
		Serial.print(micronetData->vcc.value);
		Serial.print("V | ");
	}
	if (micronetData->log.valid)
	{
		Serial.print("LOG ");
		Serial.print(micronetData->log.value);
		Serial.print("NM | ");
	}
	if (micronetData->trip.valid)
	{
		Serial.print("TRIP ");
		Serial.print(micronetData->trip.value);
		Serial.print("NM | ");
	}
	if (micronetData->stp.valid)
	{
		Serial.print("STP ");
		Serial.print(micronetData->stp.value);
		Serial.print("degC | ");
	}

	Serial.println();
}

void MenuAbout()
{
	Serial.println("MicronetToNMEA, Version 0.1a");

	Serial.print("Serial speed : ");
	Serial.println(gConfiguration.serialSpeed);

	if (gConfiguration.attachedNetworkId != 0)
	{
		Serial.print("Attached to Micronet Network ");
		Serial.println(gConfiguration.attachedNetworkId, HEX);
	}
	else
	{
		Serial.println("No Micronet Network attached");
	}

	Serial.print("Wind speed factor = ");
	Serial.println(gConfiguration.windSpeedFactor_per);
	Serial.print("Wind direction offset = ");
	Serial.println((int)(gConfiguration.windDirectionOffset_deg));
	Serial.print("Water speed factor = ");
	Serial.println(gConfiguration.waterSpeedFactor_per);
	Serial.print("Water temperature offset = ");
	Serial.println((int)(gConfiguration.waterTemperatureOffset_C));

	Serial.println("Provides the following NMEA sentences :");
	Serial.println(" - INDPT (Depth below transducer. T121 with depth sounder required)");
	Serial.println(" - INMWV (Apparent wind. T120 required)");
	Serial.println(" - INMWV (True wind. T120 and T121 with Speedo/Temp sensor required)");
	Serial.println(" - INMTW (Water temperature. T121 with Speedo/Temp sensor required)");
	Serial.println(" - INVHW (Speed on water. T121 with Speedo/Temp sensor required)");
	Serial.println(" - INVLW (Distance log T121 with Speedo/Temp sensor required)");
}

void MenuScanNetworks()
{
	MicronetMessage_t *message;
	uint32_t nidArray[MAX_SCANNED_NETWORKS] =
	{ 0 };
	int16_t rssiArray[MAX_SCANNED_NETWORKS];

	Serial.print("Scanning Micronet networks for 5 seconds ... ");

	gMessageFifo.ResetFifo();
	unsigned long startTime = millis();
	do
	{
		if ((message = gMessageFifo.Peek()) != nullptr)
		{
			// TODO : check that header is valid
			uint32_t nid = gMicronetDecoder.GetNetworkId(message);
			int16_t rssi = message->rssi;
			for (int i = 0; i < MAX_SCANNED_NETWORKS; i++)
			{
				if (nidArray[i] == 0)
				{
					nidArray[i] = nid;
					rssiArray[i] = rssi;
					break;
				}
				else if (nidArray[i] == nid)
				{
					if (rssi > rssiArray[i])
					{
						rssiArray[i] = rssi;
					}
					break;
				}
				else
				{
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
			gMessageFifo.DeleteMessage();
		}
	} while ((millis() - startTime) < 5000);

	Serial.println("done");
	Serial.println("");

	if (nidArray[0] != 0)
	{
		Serial.println("List of scanned networks :");
		Serial.println("");
		for (int i = 0; i < MAX_SCANNED_NETWORKS; i++)
		{
			if (nidArray[i] == 0)
			{
				break;
			}
			Serial.print("Network ");
			Serial.print(i);
			Serial.print(" - ");
			Serial.print(nidArray[i], HEX);
			Serial.print(" (");
			if (rssiArray[i] < 70)
				Serial.print("very strong");
			else if (rssiArray[i] < 80)
				Serial.print("strong");
			else if (rssiArray[i] < 90)
				Serial.print("normal");
			else
				Serial.print("low");
			Serial.println(")");
		}
	}
	else
	{
		Serial.println("/!\\ No Micronet network found /!\\");
		Serial.println("Check that your Micronet network is powered on.");
	}
}

void MenuAttachNetwork()
{
	char input[16], c;
	uint32_t charIndex = 0;

	Serial.print("Enter Network ID to attach to : ");

	do
	{
		if (Serial.available())
		{
			c = Serial.read();
			if (c == 0x0d)
			{
				Serial.println("");
				break;
			}
			else if ((c == 0x08) && (charIndex > 0))
			{
				charIndex--;
				Serial.print(c);
				Serial.print(" ");
				Serial.print(c);
			}
			else if (charIndex < sizeof(input))
			{
				input[charIndex++] = c;
				Serial.print(c);
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
		Serial.println("Invalid Network ID entered, ignoring input.");
	}
	else
	{
		gConfiguration.attachedNetworkId = newNetworkId;
		Serial.print("Now attached to NetworkID ");
		Serial.println(newNetworkId, HEX);
		gConfiguration.SaveToEeprom();
	}
}

void MenuConvertToNmea()
{
	bool exitNmeaLoop = false;
	char nmeaSentence[256];

	if (gConfiguration.attachedNetworkId == 0)
	{
		Serial.println("No Micronet network has been attached.");
		Serial.println("Scan and attach a Micronet network first.");
		return;
	}

	Serial.println("");
	Serial.println("Starting Micronet to NMEA0183 conversion.");
	Serial.println("Press ESC key at any time to stop conversion and come back to menu.");
	Serial.println("");

	gMessageFifo.ResetFifo();

	MicronetMessage_t *message;
	do
	{
		if ((message = gMessageFifo.Peek()) != nullptr)
		{
			if (gMicronetDecoder.VerifyHeaderCrc(message))
			{
				if (gMicronetDecoder.GetNetworkId(message) == gConfiguration.attachedNetworkId)
				{
					gMicronetDecoder.DecodeMessage(message);

					if (gNmeaEncoder.EncodeMWV_R(gMicronetDecoder.GetCurrentData(), nmeaSentence))
						Serial.print(nmeaSentence);
					if (gNmeaEncoder.EncodeMWV_T(gMicronetDecoder.GetCurrentData(), nmeaSentence))
						Serial.print(nmeaSentence);
					if (gNmeaEncoder.EncodeDPT(gMicronetDecoder.GetCurrentData(), nmeaSentence))
						Serial.print(nmeaSentence);
					if (gNmeaEncoder.EncodeMTW(gMicronetDecoder.GetCurrentData(), nmeaSentence))
						Serial.print(nmeaSentence);
					if (gNmeaEncoder.EncodeVLW(gMicronetDecoder.GetCurrentData(), nmeaSentence))
						Serial.print(nmeaSentence);
					if (gNmeaEncoder.EncodeVHW(gMicronetDecoder.GetCurrentData(), nmeaSentence))
						Serial.print(nmeaSentence);
					if (gMicronetDecoder.GetCurrentData()->calibrationUpdated)
					{
						UpdateAndSaveCalibration();
					}
				}
			}
			gMessageFifo.DeleteMessage();
		}

		while (Serial.available() > 0)
		{
			if (Serial.read() == 0x1b)
			{
				Serial.println("ESC key pressed, stopping conversion.");
				exitNmeaLoop = true;
			}
		}

		yield();
	} while (!exitNmeaLoop);
}

void MenuScanAllMicronetTraffic()
{
	bool exitSniffLoop = false;

	Serial.println("Starting Micronet traffic scanning.");
	Serial.println("Press ESC key at any time to stop scanning and come back to menu.");
	Serial.println("");

	gMessageFifo.ResetFifo();

	MicronetMessage_t *message;
	do
	{
		if ((message = gMessageFifo.Peek()) != nullptr)
		{
			if (gMicronetDecoder.VerifyHeaderCrc(message))
			{
				PrintRawMessage(message);
			}
			gMessageFifo.DeleteMessage();
		}

		while (Serial.available() > 0)
		{
			if (Serial.read() == 0x1b)
			{
				Serial.println("ESC key pressed, stopping scan.");
				exitSniffLoop = true;
			}
		}
		yield();
	} while (!exitSniffLoop);
}

void UpdateAndSaveCalibration()
{
	MicronetData_t *data = gMicronetDecoder.GetCurrentData();
	gConfiguration.waterSpeedFactor_per = data->waterSpeedFactor_per;
	gConfiguration.waterTemperatureOffset_C = data->waterTemperatureOffset_C;
	gConfiguration.depthOffset_m = data->depthOffset_m;
	gConfiguration.windSpeedFactor_per = data->windSpeedFactor_per;
	gConfiguration.windDirectionOffset_deg = data->windDirectionOffset_deg;
	gConfiguration.headingOffset_deg = data->headingOffset_deg;
	gConfiguration.magneticVariation_deg = data->magneticVariation_deg;
	gConfiguration.windShift = data->windShift;

	gConfiguration.SaveToEeprom();
}
