/*
 * RfDriver.cpp
 *
 *  Created on: 18 sept. 2021
 *      Author: Ronan
 */

#include "RfDriver.h"
#include "BoardConfig.h"
#include <Arduino.h>

#include <TeensyTimerTool.h>

using namespace TeensyTimerTool;

OneShotTimer timerInt;

RfDriver *RfDriver::rfDriver;

RfDriver::RfDriver() :
		messageFifo(nullptr), rfState(RF_STATE_RX_IDLE), messageBytesSent(0), frequencyOffset_mHz(0)
{
}

RfDriver::~RfDriver()
{
}

bool RfDriver::Init(MicronetMessageFifo *messageFifo, float frequencyOffset_mHz)
{
	if (!cc1101Driver.IsConnected())
	{
		return false;
	}

	this->frequencyOffset_mHz = frequencyOffset_mHz;
	this->messageFifo = messageFifo;
	rfDriver = this;

	timerInt.begin(TimerHandler);

	cc1101Driver.Init();
	cc1101Driver.SetFrequency(MICRONET_RF_CENTER_FREQUENCY_MHZ + frequencyOffset_mHz); // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
	cc1101Driver.SetDeviation(MICRONET_RF_DEVIATION_KHZ); // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
	cc1101Driver.SetRate(MICRONET_RF_BAUDRATE_BAUD / 1000.0f);
	cc1101Driver.SetBw(250);
	cc1101Driver.SetSyncWord(0x55, 0x99); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
	cc1101Driver.SetLengthConfig(0); // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
	cc1101Driver.SetPacketLength(60); // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
	cc1101Driver.SetPQT(4);

	return true;
}

void RfDriver::SetFrequencyOffset(float offset_MHz)
{
	frequencyOffset_mHz = offset_MHz;
}

void RfDriver::SetFrequency(float freq_MHz)
{
	cc1101Driver.SetFrequency(freq_MHz + frequencyOffset_mHz);
}

void RfDriver::SetDeviation(float freq_KHz)
 {
	cc1101Driver.SetDeviation(freq_KHz);
 }

void RfDriver::SetBandwidth(float bw_KHz)
{
	cc1101Driver.SetBw(bw_KHz);
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
	nbBytes = cc1101Driver.GetRxFifoLevel();
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
		cc1101Driver.ReadRxFifo(message.data + dataOffset, nbBytes);
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
				cc1101Driver.SetPacketLength(packetLength);
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
	message.rssi = cc1101Driver.GetRssi();
	message.startTime_us = startTime_us;
	message.endTime_us = endTime_us;
	messageFifo->Push(message);
}

void RfDriver::GDO0TxCallback()
{
	int bytesInFifo = 17; // Corresponds to the FIFO threshold of 0x0b

	while ((bytesInFifo < 62) && (messageBytesSent < messageToTransmit.len))
	{
		cc1101Driver.WriteTxFifo(messageToTransmit.data[messageBytesSent++]);
		bytesInFifo++;
	}

	if (messageBytesSent >= messageToTransmit.len)
	{
		rfState = RF_STATE_TX_LAST_TRANSMIT;
		cc1101Driver.DeIrqOnTxFifoEmpty();
	}
}

void RfDriver::GDO0LastTxCallback()
{
	cc1101Driver.SetSidle();
	cc1101Driver.FlushTxFifo();
	RestartReception();
}

void RfDriver::RestartReception()
{
	rfState = RF_STATE_RX_IDLE;
	cc1101Driver.SetSidle();
	cc1101Driver.SetSyncMode(2);
	cc1101Driver.SetLengthConfig(0);
	cc1101Driver.SetPacketLength(60);
	cc1101Driver.FlushRxFifo();
	cc1101Driver.SetFifoThreshold(CC1101_RXFIFOTHR_16);
	cc1101Driver.IrqOnRxFifoThreshold();
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
	cc1101Driver.SetSidle();
	cc1101Driver.SetSyncMode(0);
	cc1101Driver.SetLengthConfig(2);
	cc1101Driver.FlushTxFifo();

	// Fill FIFO with preamble + sync byte
	int bytesInFifo = 0;
	for (bytesInFifo = 0; bytesInFifo < MICRONET_RF_PREAMBLE_LENGTH; bytesInFifo++)
		cc1101Driver.WriteTxFifo(0x55);
	cc1101Driver.WriteTxFifo(MICRONET_RF_SYNC_BYTE);

	messageBytesSent = 0;
	while ((bytesInFifo < 62) && (messageBytesSent < messageToTransmit.len))
	{
		cc1101Driver.WriteTxFifo(messageToTransmit.data[messageBytesSent++]);
		bytesInFifo++;
	}

	if (messageBytesSent < messageToTransmit.len)
	{
		rfState = RF_STATE_TX_TRANSMITTING;
		cc1101Driver.SetFifoThreshold(CC1101_TXFIFOTHR_17);
		cc1101Driver.IrqOnTxFifoLow();
	}
	else
	{
		rfState = RF_STATE_TX_LAST_TRANSMIT;
		cc1101Driver.DeIrqOnTxFifoEmpty();
	}

	// Start transmission
	cc1101Driver.SetTx();
}
