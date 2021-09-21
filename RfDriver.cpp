/*
 * RfDriver.cpp
 *
 *  Created on: 18 sept. 2021
 *      Author: Ronan
 */

#include "RfDriver.h"
#include <arduino.h>

#include <TeensyTimerTool.h>

using namespace TeensyTimerTool;

OneShotTimer timerInt;

RfDriver *RfDriver::rfDriver;

RfDriver::RfDriver() :
		gdo0Pin(0), messageFifo(nullptr), rfState(RF_STATE_RX_IDLE)
{
}

RfDriver::~RfDriver()
{
}

bool RfDriver::Init(int gdo0Pin, MicronetMessageFifo *messageFifo)
{
	if (!cc1101Driver.getCC1101())
	{
		return false;
	}

	this->gdo0Pin = gdo0Pin;
	this->messageFifo = messageFifo;
	rfDriver = this;

	timerInt.begin(TimerHandler);

	cc1101Driver.Init();
	cc1101Driver.setGDO0(gdo0Pin);
	cc1101Driver.setCCMode(1); // set config for internal transmission mode.
	cc1101Driver.setModulation(0); // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
	cc1101Driver.setMHZ(869.778 - 0.034); // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
	cc1101Driver.setDeviation(34); // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
	cc1101Driver.setChannel(0); // Set the Channelnumber from 0 to 255. Default is cahnnel 0.
	cc1101Driver.setChsp(199.95); // The channel spacing is multiplied by the channel number CHAN and added to the base frequency in kHz. Value from 25.39 to 405.45. Default is 199.95 kHz.
	cc1101Driver.setRxBW(250); // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
	cc1101Driver.setDRate(76.8); // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
	cc1101Driver.setPA(12); // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
	cc1101Driver.setSyncMode(2); // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
	cc1101Driver.setSyncWord(0x55, 0x99); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
	cc1101Driver.setAdrChk(0); // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
	cc1101Driver.setAddr(0); // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
	cc1101Driver.setWhiteData(0); // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
	cc1101Driver.setPktFormat(0); // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
	cc1101Driver.setLengthConfig(0); // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
	cc1101Driver.setPacketLength(60); // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
	cc1101Driver.setCrc(0); // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
	cc1101Driver.setCRC_AF(0); // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
	cc1101Driver.setDcFilterOff(0); // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity).
	cc1101Driver.setManchester(0); // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
	cc1101Driver.setFEC(0); // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
	cc1101Driver.setPQT(4); // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
	cc1101Driver.setAppendStatus(0); // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.

	return true;
}

void RfDriver::GDO0Callback()
{
	if (rfState == RF_STATE_TX_TRANSMITTING)
	{
		GDO0TxCallback();
	}
	else if (rfState == RF_STATE_TX_LAST_TRANSMIT)
	{
		GDO0LastTxCallback();
	}
	else
	{
		GDO0RxCallback();
	}
}

void RfDriver::GDO0RxCallback()
{
	static MicronetMessage_t message;
	static int dataOffset;
	static int packetLength;
	static uint32_t startTime_us;
	int nbBytes;

	// When we reach this point, we know that a packet is under reception by CC1101. We will not wait the end of this reception and will
	// begin collecting bytes right now. This way we will be able to receive packets that are longer than FIFO size and we will instruct
	// CC1101 to change packet size on the fly as soon as we will have identified the length field
	// How many bytes are already in the FIFO ?
	nbBytes = cc1101Driver.SpiReadStatus(CC1101_RXBYTES);
	// Check for FIFO overflow
	if (nbBytes & 0x80)
	{
		// Yes : ignore current packet and restart CC1101 reception for the next packet
		RestartReception();
		return;
	}

	if (rfState == RF_STATE_RX_IDLE)
	{
		// This is a new message
		startTime_us = micros() - PREAMBLE_LENGTH_IN_US;
		packetLength = -1;
		dataOffset = 0;
	}
	// Are there new bytes in the FIFO ?
	if (nbBytes > 0)
	{
		// Yes : read them
		cc1101Driver.SpiReadBurstReg(CC1101_RXFIFO, message.data + dataOffset, nbBytes);
		dataOffset += nbBytes;
		// Check if we have reached the packet length field
		if ((rfState == RF_STATE_RX_IDLE) && (dataOffset >= (MICRONET_LEN_OFFSET_1 + 2)))
		{
			// Yes : check that this is a valid length
			if ((message.data[MICRONET_LEN_OFFSET_1] == message.data[MICRONET_LEN_OFFSET_2])
					&& (message.data[MICRONET_LEN_OFFSET_1] < MICRONET_MAX_MESSAGE_LENGTH - 3)
					&& ((message.data[MICRONET_LEN_OFFSET_1] + 2) >= MICRONET_PAYLOAD_OFFSET))
			{
				packetLength = message.data[MICRONET_LEN_OFFSET_1] + 2;
				// Update CC1101's packet length register
				cc1101Driver.setPacketLength(packetLength);
			}
			else
			{
				// The packet length is not valid : ignore current packet and restart CC1101 reception for the next packet
				RestartReception();
				return;
			}
		}
		// Continue reading as long as entire packet has not been received
	}

	if (dataOffset < packetLength)
	{
		rfState = RF_STATE_RX_RECEIVING;
		return;
	}

	uint32_t endTime_us = micros();
	// Restart CC1101 reception as soon as possible not to miss the next packet
	RestartReception();
	// Fill message structure
	message.len = packetLength;
	message.rssi = cc1101Driver.getRssi();
	message.startTime_us = startTime_us;
	message.endTime_us = endTime_us;
	messageFifo->Push(message);
}

void RfDriver::GDO0TxCallback()
{
	int bytesInFifo = 17; // Corresponds to the FIFO threshold of 0x0b

	while ((bytesInFifo < 62) && (bytesFromMessage < messageToTransmit.len))
	{
		cc1101Driver.SpiWriteReg(CC1101_TXFIFO, messageToTransmit.data[bytesFromMessage++]);
		bytesInFifo++;
	}

	Serial.println(bytesInFifo);

	if (bytesFromMessage >= messageToTransmit.len)
	{
		rfState = RF_STATE_TX_LAST_TRANSMIT;
		cc1101Driver.SpiWriteReg(CC1101_IOCFG0, 0x05);
	}
}

void RfDriver::GDO0LastTxCallback()
{
	cc1101Driver.setSidle();
	cc1101Driver.SpiStrobe(CC1101_SFTX);
	RestartReception();
}

void RfDriver::RestartReception()
{
	rfState = RF_STATE_RX_IDLE;
	cc1101Driver.setSidle();
	cc1101Driver.setSyncMode(2);
	cc1101Driver.setLengthConfig(0);
	cc1101Driver.setPacketLength(60);
	cc1101Driver.SpiStrobe(CC1101_SFRX);
	cc1101Driver.SpiWriteReg(CC1101_FIFOTHR, 0x03);
	cc1101Driver.SpiWriteReg(CC1101_IOCFG0, 0x01);
	cc1101Driver.SetRx();
}

void RfDriver::TransmitMessage(MicronetMessage_t *message, uint32_t transmitTimeUs)
{
	messageToTransmit.len = message->len;
	messageToTransmit.startTime_us = transmitTimeUs;
	memcpy(messageToTransmit.data, message->data, message->len);

	int32_t transmitDelay = transmitTimeUs - micros();
	if (transmitDelay <= 0)
	{
		return;
	}
	timerInt.trigger(transmitDelay);
}

void RfDriver::TimerHandler()
{
	rfDriver->TransmitCallback();
}

void RfDriver::TransmitCallback()
{
	// Change CC1101 configuration for transmission
	cc1101Driver.setSidle();
	cc1101Driver.setPA(12);
	cc1101Driver.setSyncMode(0);
	cc1101Driver.setLengthConfig(2);
	cc1101Driver.SpiStrobe(CC1101_SFTX);

	// Fill FIFO with preamble + sync byte
	int bytesInFifo = 0;
	for (bytesInFifo = 0; bytesInFifo < MICRONET_RF_PREAMBLE_LENGTH; bytesInFifo++)
		cc1101Driver.SpiWriteReg(CC1101_TXFIFO, 0x55);
	cc1101Driver.SpiWriteReg(CC1101_TXFIFO, MICRONET_RF_SYNC_BYTE);

	bytesFromMessage = 0;
	while ((bytesInFifo < 62) && (bytesFromMessage < messageToTransmit.len))
	{
		cc1101Driver.SpiWriteReg(CC1101_TXFIFO, messageToTransmit.data[bytesFromMessage++]);
		bytesInFifo++;
	}

	if (bytesFromMessage < messageToTransmit.len)
	{
		rfState = RF_STATE_TX_TRANSMITTING;
		cc1101Driver.SpiWriteReg(CC1101_FIFOTHR, 0x0b);
		cc1101Driver.SpiWriteReg(CC1101_IOCFG0, 0x43);
	}
	else
	{
		rfState = RF_STATE_TX_LAST_TRANSMIT;
		cc1101Driver.SpiWriteReg(CC1101_IOCFG0, 0x05);
	}

	// Start transmission
	cc1101Driver.SetTx();
}
