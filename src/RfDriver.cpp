/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Driver for CC1101                                             *
 * Author:   Ronan Demoment heavily based on ELECHOUSE's driver            *
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

#include "RfDriver.h"
#include "BoardConfig.h"
#include "Globals.h"
#include "Micronet.h"

#include <Arduino.h>
#include <TeensyTimerTool.h>

using namespace TeensyTimerTool;

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define CC1101_FIFO_MAX_SIZE 60

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

OneShotTimer timerInt;

RfDriver *RfDriver::rfDriver;

const uint8_t RfDriver::preambleAndSync[MICRONET_RF_PREAMBLE_LENGTH] =
{ MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE,
MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE,
MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE,
MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_PREAMBLE_BYTE, MICRONET_RF_SYNC_BYTE };

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

RfDriver::RfDriver() :
		messageFifo(nullptr), rfState(RF_STATE_RX_WAIT_SYNC), nextTransmitIndex(-1), messageBytesSent(0), frequencyOffset_MHz(0), freqTrackingNID(
				0)
{
	memset(transmitList, 0, sizeof(transmitList));
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

	this->frequencyOffset_MHz = frequencyOffset_mHz;
	this->messageFifo = messageFifo;
	rfDriver = this;

	timerInt.begin(TimerHandler);

	cc1101Driver.Init();
	cc1101Driver.SetFrequency(MICRONET_RF_CENTER_FREQUENCY_MHZ + frequencyOffset_mHz); // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
	cc1101Driver.SetDeviation(MICRONET_RF_DEVIATION_KHZ); // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
	cc1101Driver.SetBitrate(MICRONET_RF_BAUDRATE_BAUD / 1000.0f);
	cc1101Driver.SetBw(250);
	cc1101Driver.SetSyncWord(0x55, 0x99); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
	cc1101Driver.SetLengthConfig(0); // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
	cc1101Driver.SetPacketLength(CC1101_FIFO_MAX_SIZE); // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
	cc1101Driver.SetPQT(4);

	return true;
}

void RfDriver::SetFrequencyOffset(float offset_MHz)
{
	frequencyOffset_MHz = offset_MHz;
}

void RfDriver::SetFrequency(float frequency_MHz)
{
	cc1101Driver.SetFrequency(frequency_MHz + frequencyOffset_MHz);
}

void RfDriver::SetBandwidth(RfBandwidth_t bandwidth)
{
	switch(bandwidth)
	{
	case RF_BANDWIDTH_LOW:
		cc1101Driver.SetBw(95);
		break;
	case RF_BANDWIDTH_MEDIUM:
		cc1101Driver.SetBw(125);
		break;
	default:
		cc1101Driver.SetBw(250);
		break;
	}

}

void RfDriver::RfIsr()
{
	if ((rfState == RF_STATE_TX_TRANSMIT) || (rfState == RF_STATE_TX_LAST_TRANSMIT))
	{
		RfIsr_Tx();
	}
	else
	{
		RfIsr_Rx();
	}
}

void RfDriver::RfIsr_Tx()
{
	if (rfState == RF_STATE_TX_TRANSMIT)
	{
		int bytesInFifo;
		int bytesToLoad = transmitList[nextTransmitIndex].len - messageBytesSent;

		bytesInFifo = cc1101Driver.GetTxFifoLevel();
		if (bytesToLoad + bytesInFifo > CC1101_FIFO_MAX_SIZE)
		{
			bytesToLoad = CC1101_FIFO_MAX_SIZE - bytesInFifo;
		}

		cc1101Driver.WriteArrayTxFifo(&transmitList[nextTransmitIndex].data[messageBytesSent], bytesToLoad);
		messageBytesSent += bytesToLoad;

		if (messageBytesSent >= transmitList[nextTransmitIndex].len)
		{
			rfState = RF_STATE_TX_LAST_TRANSMIT;
			cc1101Driver.IrqOnTxFifoUnderflow();
		}
	}
	else
	{
		transmitList[nextTransmitIndex].startTime_us = 0;
		nextTransmitIndex = -1;

		RestartReception();
		ScheduleTransmit();
	}
}

void RfDriver::RfIsr_Rx()
{
	static MicronetMessage_t message;
	static int dataOffset;
	static int packetLength;
	static uint32_t startTime_us;
	uint8_t nbBytes;

	if (rfState == RF_STATE_RX_WAIT_SYNC)
	{
		// When we reach this point, we know that a packet is under reception by CC1101. We will not wait the end of this reception and will
		// begin collecting bytes right now. This way we will be able to receive packets that are longer than FIFO size and we will instruct
		// CC1101 to change packet size on the fly as soon as we will have identified the length field
		rfState = RF_STATE_RX_HEADER;
		startTime_us = micros() - PREAMBLE_LENGTH_IN_US;
		packetLength = -1;
		dataOffset = 0;

		// How many bytes are already in the FIFO ?
		nbBytes = cc1101Driver.GetRxFifoLevel();

		// Check for FIFO overflow
		if (nbBytes > 64)
		{
			// Yes : ignore current packet and restart CC1101 reception for the next packet
			RestartReception();
			return;
		}

		startTime_us -= nbBytes * BYTE_LENGTH_IN_US;
	}
	else if ((rfState == RF_STATE_RX_HEADER) || (rfState == RF_STATE_RX_PAYLOAD))
	{
		nbBytes = cc1101Driver.GetRxFifoLevel();

		// Check for FIFO overflow
		if (nbBytes > 64)
		{
			// Yes : ignore current packet and restart CC1101 reception for the next packet
			RestartReception();
			return;
		}
	}
	else
	{
		// GDO0 RX ISR is not supposed to be triggered in this state
		return;
	}

	// Are there new bytes in the FIFO ?
	if ((nbBytes > 0) && ((dataOffset < packetLength) || (packetLength < 0)))
	{
		// Yes : read them
		if (dataOffset + nbBytes > MICRONET_MAX_MESSAGE_LENGTH)
		{
			// Received data size exceeds max message size, something must have gone wrong : restart listening
			RestartReception();
			return;
		}
		cc1101Driver.ReadRxFifo(message.data + dataOffset, nbBytes);
		dataOffset += nbBytes;
		// Check if we have reached the packet length field
		if ((rfState == RF_STATE_RX_HEADER) && (dataOffset >= (MICRONET_LEN_OFFSET_1 + 2)))
		{
			rfState = RF_STATE_RX_PAYLOAD;
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
	}

	if ((dataOffset < packetLength) || (packetLength < 0))
	{
		return;
	}

	// Restart CC1101 reception as soon as possible not to miss the next packet
	RestartReception();
	// Fill message structure
	message.len = packetLength;
	message.rssi = cc1101Driver.GetRssi();
	message.startTime_us = startTime_us;
	message.endTime_us = startTime_us + PREAMBLE_LENGTH_IN_US + packetLength * BYTE_LENGTH_IN_US + GUARD_TIME_IN_US;
	message.action = MICRONET_ACTION_RF_NO_ACTION;
	messageFifo->Push(message);

	// Only perform frequency tracking if the feature has been explicitly enabled
	if (freqTrackingNID != 0)
	{
		// Only track if message is from master and for out network
		if ((message.data[MICRONET_MI_OFFSET] == MICRONET_MESSAGE_ID_MASTER_REQUEST)
				&& (gMicronetCodec.GetNetworkId(&message) == freqTrackingNID))
		{
			cc1101Driver.UpdateFreqOffset();
		}
	}
}

void RfDriver::RestartReception()
{
	cc1101Driver.SetSidle();
	cc1101Driver.FlushRxFifo();
	cc1101Driver.SetFifoThreshold(CC1101_RXFIFOTHR_16);
	cc1101Driver.IrqOnRxFifoThreshold();
	cc1101Driver.SetSyncMode(2);
	cc1101Driver.SetLengthConfig(0);
	cc1101Driver.SetPacketLength(CC1101_FIFO_MAX_SIZE);
	rfState = RF_STATE_RX_WAIT_SYNC;
	cc1101Driver.SetRx();
}

void RfDriver::Transmit(MicronetMessageFifo *txMessageFifo)
{
	MicronetMessage_t *txMessage;
	while ((txMessage = txMessageFifo->Peek()) != nullptr)
	{
		Transmit(txMessage);
		txMessageFifo->DeleteMessage();
	}
}

void RfDriver::Transmit(MicronetMessage_t *message)
{
	noInterrupts();
	int transmitIndex = GetFreeTransmitSlot();

	if (transmitIndex >= 0)
	{
		transmitList[transmitIndex].action = message->action;
		transmitList[transmitIndex].startTime_us = message->startTime_us;
		transmitList[transmitIndex].len = message->len;
		memcpy(transmitList[transmitIndex].data, message->data, message->len);
	}
	interrupts();

	ScheduleTransmit();
}

void RfDriver::ScheduleTransmit()
{
	do
	{
		int transmitIndex = GetNextTransmitIndex();
		if (transmitIndex < 0)
		{
			// No transmit to schedule : stop timer and leave
			timerInt.stop();
			return;
		}
		int32_t transmitDelay = transmitList[transmitIndex].startTime_us - micros();
		if ((transmitDelay <= 0) || (transmitDelay > 3000000))
		{
			// Transmit already in the past, or invalid : delete it and schedule the next one
			transmitList[transmitIndex].startTime_us = 0;
			continue;
		}

		// Schedule new transmit
		nextTransmitIndex = transmitIndex;
		timerInt.trigger(transmitDelay);

		return;
	} while (true);
}

int RfDriver::GetNextTransmitIndex()
{
	uint32_t minTime = 0xffffffff;
	int minIndex = -1;

	for (int i = 0; i < TRANSMIT_LIST_SIZE; i++)
	{
		if (transmitList[i].startTime_us != 0)
		{
			if (transmitList[i].startTime_us <= minTime)
			{
				minTime = transmitList[i].startTime_us;
				minIndex = i;
			}
		}
	}

	return minIndex;
}

int RfDriver::GetFreeTransmitSlot()
{
	int freeIndex = -1;

	for (int i = 0; i < TRANSMIT_LIST_SIZE; i++)
	{
		if (transmitList[i].startTime_us == 0)
		{
			freeIndex = i;
			break;
		}
	}

	return freeIndex;
}

void RfDriver::TimerHandler()
{
	rfDriver->TransmitCallback();
}

void RfDriver::TransmitCallback()
{
	if (nextTransmitIndex < 0)
	{
		RestartReception();
		return;
	}

	int32_t triggerDelay = micros() - transmitList[nextTransmitIndex].startTime_us;

	if (triggerDelay < 0)
	{
		// Depending on the Teensy version, timer may not be able to reach delay of more than
		// about 50ms. in that case we reprogram it until we reach the specified amount of time
		ScheduleTransmit();
		return;
	}

	if (transmitList[nextTransmitIndex].action == MICRONET_ACTION_RF_LOW_POWER)
	{
		transmitList[nextTransmitIndex].startTime_us = 0;
		nextTransmitIndex = -1;

		cc1101Driver.LowPower();
		rfState = RF_STATE_RX_WAIT_SYNC;

		ScheduleTransmit();
	}
	else if (transmitList[nextTransmitIndex].action == MICRONET_ACTION_RF_ACTIVE_POWER)
	{
		transmitList[nextTransmitIndex].startTime_us = 0;
		nextTransmitIndex = -1;

		cc1101Driver.ActivePower();

		ScheduleTransmit();
		RestartReception();
	}
	else if (rfState == RF_STATE_RX_WAIT_SYNC)
	{
		rfState = RF_STATE_TX_TRANSMIT;

		// Change CC1101 configuration for transmission
		cc1101Driver.SetSidle();
		cc1101Driver.IrqOnTxFifoThreshold();
		cc1101Driver.SetSyncMode(0);
		cc1101Driver.SetLengthConfig(2);
		cc1101Driver.SetFifoThreshold(CC1101_TXFIFOTHR_9);
		cc1101Driver.FlushTxFifo();

		// Fill FIFO with preamble's first byte
		cc1101Driver.WriteTxFifo(MICRONET_RF_PREAMBLE_BYTE);

		// Start transmission as soon as we have the first byte available in FIFO to minimize latency
		cc1101Driver.SetTx();

		// Fill FIFO with rest of preamble and sync byte
		cc1101Driver.WriteArrayTxFifo(static_cast<const uint8_t*>(preambleAndSync), sizeof(preambleAndSync));

		messageBytesSent = 0;
	}
	else
	{
		ScheduleTransmit();
	}
}

void RfDriver::EnableFrequencyTracking(uint32_t networkId)
{
	freqTrackingNID = networkId;
}

void RfDriver::DisableFrequencyTracking()
{
	freqTrackingNID = 0;
}
