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

#include "CC1101Driver.h"
#include "BoardConfig.h"

#include <SPI.h>
#include <Arduino.h>

#define   WRITE_BURST       0x40
#define   READ_SINGLE       0x80
#define   READ_BURST        0xC0
#define   BYTES_IN_RXFIFO   0x7F

byte freqOffset868[2] =
{ 65, 76 };
byte freqOffset915[2] =
{ 77, 79 };

uint8_t PA_TABLE[8]
{ 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

CC1101Driver::CC1101Driver() :
		rfFreq_mHz(869.840), spiSettings(SPISettings(4000000, MSBFIRST, SPI_MODE0))
{
	SPI.setMOSI(MOSI_PIN);
	SPI.setMISO(MISO_PIN);
	SPI.setSCK(SCK_PIN);
	SPI.setCS(CS0_PIN);

	pinMode(SCK_PIN, OUTPUT);
	pinMode(MOSI_PIN, OUTPUT);
	pinMode(MISO_PIN, INPUT);
	pinMode(CS0_PIN, OUTPUT);
	pinMode(GDO0_PIN, INPUT);

	digitalWrite(CS0_PIN, HIGH);
	digitalWrite(SCK_PIN, HIGH);
	digitalWrite(MOSI_PIN, LOW);

	SPI.begin();
}

CC1101Driver::~CC1101Driver()
{
	SPI.end();
}

void CC1101Driver::Reset(void)
{
	SPI.beginTransaction(spiSettings);
	digitalWrite(CS0_PIN, LOW);
	delay(1);
	digitalWrite(CS0_PIN, HIGH);
	delay(1);
	digitalWrite(CS0_PIN, LOW);
	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(CC1101_SRES);
	while (digitalRead(MISO_PIN))
		;

	SPI.endTransaction();
	digitalWrite(CS0_PIN, HIGH);
}

void CC1101Driver::Init(void)
{
	digitalWrite(CS0_PIN, HIGH);
	digitalWrite(SCK_PIN, HIGH);
	digitalWrite(MOSI_PIN, LOW);
	Reset();                    //CC1101 reset
	SetStaticConfig();            //CC1101 register config
}

void CC1101Driver::SpiWriteReg(byte addr, byte value)
{
	digitalWrite(CS0_PIN, LOW);
	Wait1us();
	SPI.beginTransaction(spiSettings);

	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(addr);
	Wait1us();
	SPI.transfer(value);
	Wait1us();

	SPI.endTransaction();
	digitalWrite(CS0_PIN, HIGH);
	Wait1us();
}

void CC1101Driver::SpiWriteBurstReg(byte addr, byte *buffer, byte num)
{
	byte i, temp;

	temp = addr | WRITE_BURST;
	digitalWrite(CS0_PIN, LOW);
	Wait1us();
	SPI.beginTransaction(spiSettings);

	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(temp);
	for (i = 0; i < num; i++)
	{
		Wait1us();
		SPI.transfer(buffer[i]);
	}
	Wait1us();

	SPI.endTransaction();
	digitalWrite(CS0_PIN, HIGH);
	Wait1us();
}

void CC1101Driver::SpiStrobe(byte strobe)
{
	digitalWrite(CS0_PIN, LOW);
	Wait1us();
	SPI.beginTransaction(spiSettings);

	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(strobe);
	Wait1us();

	SPI.endTransaction();
	digitalWrite(CS0_PIN, HIGH);
	Wait1us();
}

byte CC1101Driver::SpiReadReg(byte addr)
{
	byte temp, value;

	temp = addr | READ_SINGLE;
	digitalWrite(CS0_PIN, LOW);
	Wait1us();
	SPI.beginTransaction(spiSettings);

	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(temp);
	Wait1us();
	value = SPI.transfer(0);
	Wait1us();

	SPI.endTransaction();
	digitalWrite(CS0_PIN, HIGH);
	Wait1us();

	return value;
}

void CC1101Driver::SpiReadBurstReg(byte addr, byte *buffer, byte num)
{
	byte i, temp;

	temp = addr | READ_BURST;
	digitalWrite(CS0_PIN, LOW);
	Wait1us();
	SPI.beginTransaction(spiSettings);

	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(temp);
	for (i = 0; i < num; i++)
	{
		Wait1us();
		buffer[i] = SPI.transfer(0);
	}

	Wait1us();
	SPI.endTransaction();
	digitalWrite(CS0_PIN, HIGH);
	Wait1us();
}

byte CC1101Driver::SpiReadStatus(byte addr)
{
	byte value, temp;

	temp = addr | READ_BURST;
	digitalWrite(CS0_PIN, LOW);
	Wait1us();
	SPI.beginTransaction(spiSettings);

	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(temp);
	Wait1us();
	value = SPI.transfer(0);
	Wait1us();

	SPI.endTransaction();
	digitalWrite(CS0_PIN, HIGH);
	Wait1us();

	return value;
}

void CC1101Driver::SetFrequency(float freq_mhz)
{
	byte freq2 = 0;
	byte freq1 = 0;
	byte freq0 = 0;

	rfFreq_mHz = freq_mhz;

	for (bool i = 0; i == 0;)
	{
		if (freq_mhz >= 26)
		{
			freq_mhz -= 26;
			freq2 += 1;
		}
		else if (freq_mhz >= 0.1015625)
		{
			freq_mhz -= 0.1015625;
			freq1 += 1;
		}
		else if (freq_mhz >= 0.00039675)
		{
			freq_mhz -= 0.00039675;
			freq0 += 1;
		}
		else
		{
			i = 1;
		}
	}
	if (freq0 > 255)
	{
		freq1 += 1;
		freq0 -= 256;
	}

	SpiWriteReg(CC1101_FREQ2, freq2);
	SpiWriteReg(CC1101_FREQ1, freq1);
	SpiWriteReg(CC1101_FREQ0, freq0);

	Calibrate();
}

void CC1101Driver::Calibrate(void)
{

	if (rfFreq_mHz >= 779 && rfFreq_mHz <= 899.99)
	{
		SpiWriteReg(CC1101_FSCTRL0, map(rfFreq_mHz, 779, 899, freqOffset868[0], freqOffset868[1]));
		if (rfFreq_mHz < 861)
		{
			SpiWriteReg(CC1101_TEST0, 0x0B);
		}
		else
		{
			SpiWriteReg(CC1101_TEST0, 0x09);
			int s = SpiReadStatus(CC1101_FSCAL2);
			if (s < 32)
			{
				SpiWriteReg(CC1101_FSCAL2, s + 32);
			}
		}
	}
	else if (rfFreq_mHz >= 900 && rfFreq_mHz <= 928)
	{
		SpiWriteReg(CC1101_FSCTRL0, map(rfFreq_mHz, 900, 928, freqOffset915[0], freqOffset915[1]));
		SpiWriteReg(CC1101_TEST0, 0x09);
		int s = SpiReadStatus(CC1101_FSCAL2);
		if (s < 32)
		{
			SpiWriteReg(CC1101_FSCAL2, s + 32);
		}
	}
}

bool CC1101Driver::IsConnected(void)
{
	if (SpiReadStatus(0x31) > 0)
		return 1;
	else
		return 0;
}

void CC1101Driver::SetSyncWord(byte sh, byte sl)
{
	SpiWriteReg(CC1101_SYNC1, sh);
	SpiWriteReg(CC1101_SYNC0, sl);
}

void CC1101Driver::SetPQT(uint8_t pqt)
{
	SpiWriteReg(CC1101_PKTCTRL1, pqt << 5);
}

void CC1101Driver::SetLengthConfig(uint8_t config)
{
	uint8_t PKTCTRL0 = SpiReadReg(CC1101_PKTCTRL0) & 0xfc;
	SpiWriteReg(CC1101_PKTCTRL0, PKTCTRL0 | config);
}

void CC1101Driver::SetPacketLength(byte v)
{
	SpiWriteReg(CC1101_PKTLEN, v);
}

int CC1101Driver::GetRxFifoLevel()
{
	return SpiReadStatus(CC1101_RXBYTES);
}

int CC1101Driver::GetTxFifoLevel()
{
	return SpiReadStatus(CC1101_TXBYTES);
}

void CC1101Driver::ReadRxFifo(uint8_t *buffer, int nbBytes)
{
	SpiReadBurstReg(CC1101_RXFIFO, buffer, nbBytes);
}

void CC1101Driver::WriteTxFifo(uint8_t data)
{
	SpiWriteReg(CC1101_TXFIFO, data);
}

void CC1101Driver::DeIrqOnTxFifoEmpty()
{
	SpiWriteReg(CC1101_IOCFG0, 0x05);
}

void CC1101Driver::IrqOnTxFifoLow()
{
	SpiWriteReg(CC1101_IOCFG0, 0x43);
}

void CC1101Driver::IrqOnRxFifoThreshold()
{
	SpiWriteReg(CC1101_IOCFG0, 0x01);
}

void CC1101Driver::SetFifoThreshold(uint8_t fifoThreshold)
{
	SpiWriteReg(CC1101_FIFOTHR, fifoThreshold);
}

void CC1101Driver::FlushRxFifo()
{
	SpiStrobe(CC1101_SFRX);
}

void CC1101Driver::FlushTxFifo()
{
	SpiStrobe(CC1101_SFTX);
}

void CC1101Driver::SetBw(float bw_kHz)
{
	int s1 = 3;
	int s2 = 3;
	for (int i = 0; i < 3; i++)
	{
		if (bw_kHz > 101.5625)
		{
			bw_kHz /= 2;
			s1--;
		}
		else
		{
			i = 3;
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (bw_kHz > 58.1)
		{
			bw_kHz /= 1.25;
			s2--;
		}
		else
		{
			i = 3;
		}
	}
	s1 *= 64;
	s2 *= 16;

	uint8_t MDMCFG4 = SpiReadReg(CC1101_MDMCFG4) & 0x0f;
	SpiWriteReg(CC1101_MDMCFG4, MDMCFG4 | s1 | s2);
}

void CC1101Driver::SetRate(float br)
{
	byte m4DaRa;
	float c = br;
	byte MDMCFG3 = 0;
	if (c > 1621.83)
	{
		c = 1621.83;
	}
	if (c < 0.0247955)
	{
		c = 0.0247955;
	}
	m4DaRa = 0;
	for (int i = 0; i < 20; i++)
	{
		if (c <= 0.0494942)
		{
			c = c - 0.0247955;
			c = c / 0.00009685;
			MDMCFG3 = c;
			float s1 = (c - MDMCFG3) * 10;
			if (s1 >= 5)
			{
				MDMCFG3++;
			}
			i = 20;
		}
		else
		{
			m4DaRa++;
			c = c / 2;
		}
	}

	uint8_t MDMCFG4 = SpiReadReg(CC1101_MDMCFG4) & 0xf0;
	SpiWriteReg(CC1101_MDMCFG4, MDMCFG4 | m4DaRa);
	SpiWriteReg(CC1101_MDMCFG3, MDMCFG3);
}

void CC1101Driver::SetDeviation(float d)
{
	float f = 1.586914;
	float v = 0.19836425;
	int c = 0;
	if (d > 380.859375)
	{
		d = 380.859375;
	}
	if (d < 1.586914)
	{
		d = 1.586914;
	}
	for (int i = 0; i < 255; i++)
	{
		f += v;
		if (c == 7)
		{
			v *= 2;
			c = -1;
			i += 8;
		}
		if (f >= d)
		{
			c = i;
			i = 255;
		}
		c++;
	}
	SpiWriteReg(21, c);
}

void CC1101Driver::SetSyncMode(uint8_t mode)
{
	// TODO : Add enum for mode
	SpiWriteReg(CC1101_MDMCFG2, mode);
}

void CC1101Driver::SetTx(void)
{
	SpiStrobe(CC1101_SIDLE);
	SpiStrobe(CC1101_STX);
}

void CC1101Driver::SetRx(void)
{
	SpiStrobe(CC1101_SIDLE);
	SpiStrobe(CC1101_SRX);        //start receive
}

void CC1101Driver::SetSidle(void)
{
	SpiStrobe(CC1101_SIDLE);
}

int CC1101Driver::GetRssi(void)
{
	int rssi;
	rssi = SpiReadStatus(CC1101_RSSI);
	if (rssi >= 128)
	{
		rssi = (rssi - 256) / 2 - 74;
	}
	else
	{
		rssi = (rssi / 2) - 74;
	}
	return rssi;
}

byte CC1101Driver::GetLqi(void)
{
	byte lqi;
	lqi = SpiReadStatus(CC1101_LQI);
	return lqi;
}

void CC1101Driver::SetStaticConfig(void)
{
	SpiWriteReg(CC1101_FSCTRL1, 0x06);

	// CC Mode
	SpiWriteReg(CC1101_IOCFG2, 0x0B);
	SpiWriteReg(CC1101_IOCFG0, 0x06);
	SpiWriteReg(CC1101_PKTCTRL0, 0x05);
	SpiWriteReg(CC1101_MDMCFG3, 0xF8);

	// 2-FSK
	SetSyncMode(2);
	SpiWriteReg(CC1101_FREND0, 0x10);

	// RF Frequency
	SetFrequency(rfFreq_mHz);

	SpiWriteReg(CC1101_MDMCFG1, 0x02);
	SpiWriteReg(CC1101_MDMCFG0, 0xF8);
	SpiWriteReg(CC1101_CHANNR, 0x00);
	SpiWriteReg(CC1101_DEVIATN, 0x47);
	SpiWriteReg(CC1101_FREND1, 0x56);
	SpiWriteReg(CC1101_MCSM0, 0x18);
	SpiWriteReg(CC1101_FOCCFG, 0x16);
	SpiWriteReg(CC1101_BSCFG, 0x1C);
	SpiWriteReg(CC1101_AGCCTRL2, 0xC7);
	SpiWriteReg(CC1101_AGCCTRL1, 0x00);
	SpiWriteReg(CC1101_AGCCTRL0, 0xB2);
	SpiWriteReg(CC1101_FSCAL3, 0xE9);
	SpiWriteReg(CC1101_FSCAL2, 0x2A);
	SpiWriteReg(CC1101_FSCAL1, 0x00);
	SpiWriteReg(CC1101_FSCAL0, 0x1F);
	SpiWriteReg(CC1101_FSTEST, 0x59);
	SpiWriteReg(CC1101_TEST2, 0x81);
	SpiWriteReg(CC1101_TEST1, 0x35);
	SpiWriteReg(CC1101_TEST0, 0x09);
	SpiWriteReg(CC1101_PKTCTRL1, 0x80);
	SpiWriteReg(CC1101_ADDR, 0x00);
	SpiWriteReg(CC1101_PKTLEN, 0x00);

	SpiWriteBurstReg(CC1101_PATABLE, PA_TABLE, 8);
}

void CC1101Driver::Wait1us()
{
	uint32_t now = micros();
	while (micros() < (now + 10))
	{
	}
}
