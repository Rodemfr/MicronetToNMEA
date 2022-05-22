/*
 * RfDriver.cpp
 *
 *  Created on: 18 sept. 2021
 *      Author: Ronan
 */

#include "RfDriver.h"
#include "BoardConfig.h"

#include <Arduino.h>
#include <SPI.h>
#include <TeensyTimerTool.h>

#define WRITE_BURST     0x40 // CC1101 Burst write flag
#define READ_SINGLE     0x80 // CC1101 Read flag
#define READ_BURST      0xC0 // CC1101 Burst read flag
#define FIFO_SIZE       64   // CC1101 RX/TX FIFO size

// Regiser map
#define REG_IOCFG2         0x00
#define REG_IOCFG1         0x01
#define REG_IOCFG0         0x02
#define REG_FIFOTHR        0x03
#define REG_SYNC1          0x04
#define REG_SYNC0          0x05
#define REG_PKTLEN         0x06
#define REG_PKTCTRL1       0x07
#define REG_PKTCTRL0       0x08
#define REG_ADDR           0x09
#define REG_CHANNR         0x0a
#define REG_FSCTRL1        0x0b
#define REG_FSCTRL0        0x0c
#define REG_FREQ2          0x0d
#define REG_FREQ1          0x0e
#define REG_FREQ0          0x0f
#define REG_MDMCFG4        0x10
#define REG_MDMCFG3        0x11
#define REG_MDMCFG2        0x12
#define REG_MDMCFG1        0x13
#define REG_MDMCFG0        0x14
#define REG_DEVIATN        0x15
#define REG_MCSM2          0x16
#define REG_MCSM1          0x17
#define REG_MCSM0          0x18
#define REG_FOCCFG         0x19
#define REG_BSCFG          0x1a
#define REG_AGCCTRL2       0x1b
#define REG_AGCCTRL1       0x1c
#define REG_AGCCTRL0       0x1d
#define REG_WOREVT1        0x1e
#define REG_WOREVT0        0x1f
#define REG_WORCTRL        0x20
#define REG_FREND1         0x21
#define REG_FREND0         0x22
#define REG_FSCAL3         0x23
#define REG_FSCAL2         0x24
#define REG_FSCAL1         0x25
#define REG_FSCAL0         0x26
#define REG_RCCTRL1        0x27
#define REG_RCCTRL0        0x28
#define REG_FSTEST         0x29
#define REG_PTEST          0x2a
#define REG_AGCTEST        0x2b
#define REG_TEST2          0x2c
#define REG_TEST1          0x2d
#define REG_TEST0          0x2e
#define REG_PARTNUM        0x30
#define REG_VERSION        0x31
#define REG_FREQEST        0x32
#define REG_LQI            0x33
#define REG_RSSI           0x34
#define REG_MARCSTATE      0x35
#define REG_WORTIME1       0x36
#define REG_WORTIME0       0x37
#define REG_PKTSTATUS      0x38
#define REG_VCO_VC_DAC     0x39
#define REG_TXBYTES        0x3a
#define REG_RXBYTES        0x3b
#define REG_RCCTRL1_STATUS 0x3c
#define REG_RCCTRL0_STATUS 0x3d
#define REG_PATABLE        0x3e
#define REG_TXFIFO         0x3f
#define REG_RXFIFO         0x3f

// Strobe registers
#define SREG_SRES          0x30
#define SREG_SFSTXON       0x31
#define SREG_SXOFF         0x32
#define SREG_SCAL          0x33
#define SREG_SRX           0x34
#define SREG_STX           0x35
#define SREG_SIDLE         0x36
#define SREG_SAFC          0x37
#define SREG_SWOR          0x38
#define SREG_SPWD          0x39
#define SREG_SFRX          0x3A
#define SREG_SFTX          0x3B
#define SREG_SWORRST       0x3C
#define SREG_SNOP          0x3D

#define CC1101_XTAL_FREQ_MHZ       26.0f
#define CC1101_DEFAULT_PACKET_SIZE 60

uint8_t CC1101_PA_TABLE[8] =
{ 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

using namespace TeensyTimerTool;

OneShotTimer timerInt;

RfDriver *RfDriver::rfDriver;

RfDriver::RfDriver() :
		gdo0Pin(0), sckPin(0), mosiPin(0), misoPin(0), csPin(0), spiInit(false), spiSettings(
				SPISettings(4000000, MSBFIRST, SPI_MODE0)), messageFifo(nullptr), rfState(RF_STATE_RX_IDLE), messageBytesSent(0), frequencyOffset_mHz(
				0)
{
}

RfDriver::~RfDriver()
{
}

bool RfDriver::Init(int gdo0Pin, int sckPin, int misoPin, int mosiPin, int csPin, MicronetMessageFifo *messageFifo,
		float frequencyOffset_mHz)
{
	this->frequencyOffset_mHz = frequencyOffset_mHz;
	this->gdo0Pin = gdo0Pin;
	this->messageFifo = messageFifo;
	rfDriver = this;

	if (spiInit)
	{
		SPI.end();
		spiInit = false;
	}

	this->sckPin = sckPin;
	this->misoPin = misoPin;
	this->mosiPin = mosiPin;
	this->csPin = csPin;

	SPI.setSCK(sckPin);
	SPI.setMISO(misoPin);
	SPI.setMOSI(mosiPin);
	SPI.setCS(csPin);

	// Init SPI pin & driver
	this->csPin = csPin;
	pinMode(sckPin, OUTPUT);
	pinMode(misoPin, OUTPUT);
	pinMode(mosiPin, INPUT);
	pinMode(csPin, OUTPUT);
	SPI.begin();

	// Prepare GDO0 pin
	this->gdo0Pin = gdo0Pin;
	pinMode(gdo0Pin, OUTPUT);

	spiInit = true;

	// Check if CC1101 is present
	if (SpiReadStatus(REG_VERSION) == 0)
	{
		SPI.end();
		spiInit = false;
		return false;
	}

	// Init timer
	timerInt.begin(TimerHandler);

	SpiWriteReg(REG_FIFOTHR, 0x03);                // FIFO threshold = 16 bytes
	SpiWriteReg(REG_IOCFG0, 0x01);                 // GDO0 : RX FIFO above threshold
	SpiWriteReg(REG_IOCFG2, 0x0B);                 // GDO2 : synchronous serial clock (not used here)
	SpiWriteReg(REG_PKTCTRL0, 0x00);               // Normal FIFO Mode + No CRC + fixed packet length
	SpiWriteReg(REG_PKTCTRL1, 0x00);               // PQT = 0, Append status disabled
	SpiWriteReg(REG_PKTLEN, CC1101_DEFAULT_PACKET_SIZE); // Set default packet size. Let some space for extra RSSI/LQI bytes
	SpiWriteReg(REG_MDMCFG4, 0x65);                // 270MHz bandwidth + 76767baud@26MHz
	SpiWriteReg(REG_MDMCFG3, 0x75);                // 76767baud@26MHz
	SpiWriteReg(REG_MDMCFG2, 0x02);                // DC filter + 2FSK + 16bit sync word
	SpiWriteReg(REG_MDMCFG1, 0x03);                // 300kHz channel spacing
	SpiWriteReg(REG_MDMCFG0, 0x7a);                // 300kHz channel spacing
	SpiWriteReg(REG_FREND0, 0x10);                 // PA table config
	SpiWriteReg(REG_DEVIATN, 0x43);                // 2-FSK deviation = 34.912kHz@26MHz
	SpiWriteReg(REG_CHANNR, 0x00);                 // Channel 0
	SpiWriteReg(REG_SYNC1, 0x55);                  // Sync word
	SpiWriteReg(REG_SYNC0, MICRONET_RF_SYNC_BYTE); // Sync word
	SpiWriteReg(REG_ADDR, 0x00);                   // Address for packet filtering (unused)
	SpiWriteReg(REG_FSCTRL0, 0x00);                // 868-915MHz
	SpiWriteReg(REG_FSCTRL1, 0x06);                // 868-915MHz
	SpiWriteBurstReg(REG_PATABLE, CC1101_PA_TABLE, 8); // Max TX power
    SpiWriteReg(REG_FREND1, 0x56);
    SpiWriteReg(REG_MCSM0 , 0x18);
    SpiWriteReg(REG_FOCCFG, 0x16);
    SpiWriteReg(REG_BSCFG, 0x6C);
    SpiWriteReg(REG_AGCCTRL2, 0x03);
    SpiWriteReg(REG_AGCCTRL1, 0x40);
    SpiWriteReg(REG_AGCCTRL0, 0x91);
    SpiWriteReg(CC1101_FSCAL3,   0xE9);
    SpiWriteReg(CC1101_FSCAL2,   0x2A);
    SpiWriteReg(CC1101_FSCAL1,   0x00);
    SpiWriteReg(CC1101_FSCAL0,   0x1F);
    SpiWriteReg(CC1101_FSTEST,   0x59);
    SpiWriteReg(CC1101_TEST2,    0x81);
    SpiWriteReg(CC1101_TEST1,    0x35);
    SpiWriteReg(CC1101_TEST0,    0x09);
    SpiWriteReg(CC1101_PKTCTRL1, 0x04);
    SpiWriteReg(CC1101_ADDR,     0x00);
    SpiWriteReg(CC1101_PKTLEN,   0x00);


	SetFrequency(MICRONET_RF_CENTER_FREQUENCY_MHZ + frequencyOffset_mHz);

	return true;
}

void RfDriver::SetFrequencyOffset(float offset_MHz)
{
	frequencyOffset_mHz = offset_MHz;
}

void RfDriver::SetFrequency(float freq_MHz)
{
	uint32_t value;

	value = (freq_MHz + frequencyOffset_mHz) * 65536.0f / CC1101_XTAL_FREQ_MHZ;

	SpiWriteReg(REG_FREQ2, (value >> 16) & 0xff);
	SpiWriteReg(REG_FREQ1, (value >> 8) & 0xff);
	SpiWriteReg(REG_FREQ0, value & 0xff);
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
	nbBytes = SpiReadStatus(REG_RXBYTES);
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
		SpiReadBurstReg(REG_RXFIFO, message.data + dataOffset, nbBytes);
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
				SpiWriteReg(REG_PKTLEN, packetLength);
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
	message.rssi = GetRssi();
	message.startTime_us = startTime_us;
	message.endTime_us = endTime_us;
	messageFifo->Push(message);
}

void RfDriver::GDO0TxCallback()
{
	int bytesInFifo = 17; // Corresponds to the FIFO threshold of 0x0b

	while ((bytesInFifo < 62) && (messageBytesSent < messageToTransmit.len))
	{
		SpiWriteReg(REG_TXFIFO, messageToTransmit.data[messageBytesSent++]);
		bytesInFifo++;
	}

	if (messageBytesSent >= messageToTransmit.len)
	{
		rfState = RF_STATE_TX_LAST_TRANSMIT;
		SpiWriteReg(REG_IOCFG0, 0x05);
	}
}

void RfDriver::GDO0LastTxCallback()
{
	SetIdle();
	SpiStrobe(SREG_SFTX);
	RestartReception();
}

void RfDriver::RestartReception()
{
	rfState = RF_STATE_RX_IDLE;
	SetIdle();
	SpiWriteReg(REG_MDMCFG2, 0x02);  // Enable sync word detection
	SpiWriteReg(REG_PKTCTRL0, 0x00); // Fixed packet length
	SpiWriteReg(REG_PKTLEN, CC1101_DEFAULT_PACKET_SIZE);
	ResetRxFifo();
	SpiWriteReg(REG_FIFOTHR, 0x03); // FIFO threshold = 16 bytes
	SpiWriteReg(REG_IOCFG0, 0x01); // GDO0 : RX FIFO above threshold
	SetRx();
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
	SetIdle();
	SpiWriteReg(REG_MDMCFG2, 0x00);  // Disable sync word detection
	SpiWriteReg(REG_PKTCTRL0, 0x02); // Infinite packet length
	ResetTxFifo();

	// Fill FIFO with preamble + sync byte
	int bytesInFifo = 0;
	for (bytesInFifo = 0; bytesInFifo < MICRONET_RF_PREAMBLE_LENGTH; bytesInFifo++)
		SpiWriteReg(REG_TXFIFO, 0x55);
	SpiWriteReg(REG_TXFIFO, MICRONET_RF_SYNC_BYTE);

	messageBytesSent = 0;
	// FIXME : use FIFO size constant
	while ((bytesInFifo < 62) && (messageBytesSent < messageToTransmit.len))
	{
		SpiWriteReg(REG_TXFIFO, messageToTransmit.data[messageBytesSent++]);
		bytesInFifo++;
	}

	if (messageBytesSent < messageToTransmit.len)
	{
		rfState = RF_STATE_TX_TRANSMITTING;
		SpiWriteReg(REG_FIFOTHR, 0x0b); // TX FIFO threshold = 17 bytes
		SpiWriteReg(REG_IOCFG0, 0x43); // GDO0 asserts when TX FIFO under threshold
	}
	else
	{
		rfState = RF_STATE_TX_LAST_TRANSMIT;
		// GDO0 asserts when TX FIFO is empty
		SpiWriteReg(REG_IOCFG0, 0x05);
	}

	// Start transmission
	SetTx();
}

void RfDriver::SpiWriteReg(uint8_t addr, uint8_t value)
{
	SPI.beginTransaction(spiSettings);
	digitalWrite(csPin, LOW);
	while (digitalRead(misoPin))
		;
	SPI.transfer(addr);
	SPI.transfer(value);
	digitalWrite(csPin, HIGH);
	SPI.endTransaction();
}

void RfDriver::SpiWriteBurstReg(uint8_t addr, uint8_t *buffer, uint8_t nbBytes)
{
	uint8_t i, temp;

	SPI.beginTransaction(spiSettings);
	temp = addr | WRITE_BURST;
	digitalWrite(csPin, LOW);
	while (digitalRead(misoPin))
		;
	SPI.transfer(temp);
	for (i = 0; i < nbBytes; i++)
	{
		SPI.transfer(buffer[i]);
	}
	digitalWrite(csPin, HIGH);
	SPI.endTransaction();
}

void RfDriver::SpiStrobe(uint8_t strobe)
{
	SPI.beginTransaction(spiSettings);
	digitalWrite(csPin, LOW);
	while (digitalRead(misoPin))
		;
	SPI.transfer(strobe);
	digitalWrite(csPin, HIGH);
	SPI.endTransaction();
}

uint8_t RfDriver::SpiReadReg(uint8_t addr)
{
	uint8_t temp, value;

	SPI.beginTransaction(spiSettings);
	temp = addr | READ_SINGLE;
	digitalWrite(csPin, LOW);
	while (digitalRead(misoPin))
		;
	SPI.transfer(temp);
	value = SPI.transfer(0);
	digitalWrite(csPin, HIGH);
	SPI.endTransaction();

	return value;
}

void RfDriver::SpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t nbBytes)
{
	uint8_t i, temp;

	SPI.beginTransaction(spiSettings);
	temp = addr | READ_BURST;
	digitalWrite(csPin, LOW);
	while (digitalRead(misoPin))
		;
	SPI.transfer(temp);
	for (i = 0; i < nbBytes; i++)
	{
		buffer[i] = SPI.transfer(0);
	}
	digitalWrite(csPin, HIGH);
	SPI.endTransaction();
}

uint8_t RfDriver::SpiReadStatus(uint8_t addr)
{
	uint8_t value, temp;
	SPI.beginTransaction(spiSettings);
	temp = addr | READ_BURST;
	digitalWrite(csPin, LOW);
	while (digitalRead(misoPin))
		;
	SPI.transfer(temp);
	value = SPI.transfer(0);
	digitalWrite(csPin, HIGH);
	SPI.endTransaction();

	return value;
}

int RfDriver::GetRssi(void)
{
	int rssi = SpiReadStatus(REG_RSSI);

	if (rssi >= 128)
		rssi = (rssi - 256) / 2 - 74;
	else
		rssi = (rssi / 2) - 74;

	return rssi;
}

void RfDriver::SetIdle()
{
	SpiStrobe(SREG_SIDLE);
}

void RfDriver::SetRx()
{
	SpiStrobe(SREG_SIDLE);
	SpiStrobe(SREG_SRX);
}

void RfDriver::SetTx()
{
	SpiStrobe(SREG_SIDLE);
	SpiStrobe(SREG_STX);
}

void RfDriver::ResetRxFifo()
{
	SpiStrobe(SREG_SFRX);
}

void RfDriver::ResetTxFifo()
{
	SpiStrobe(SREG_SFTX);
}
