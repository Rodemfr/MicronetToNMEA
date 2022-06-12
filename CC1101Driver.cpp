/*
 CC1101Driver.cpp - CC1101 module library
 Copyright (c) 2010 Michael.
 Author: Michael, <www.elechouse.com>
 Version: November 12, 2010

 This library is designed to use CC1101/CC1100 module on Arduino platform.
 CC1101/CC1100 module is an useful wireless module.Using the functions of the
 library, you can easily send and receive data by the CC1101/CC1100 module.
 Just have fun!
 For the details, please refer to the datasheet of CC1100/CC1101.
 ----------------------------------------------------------------------------------------------------------------
 cc1101 Driver for RC Switch. Mod by Little Satan. With permission to modify and publish Wilson Shen (ELECHOUSE).
 ----------------------------------------------------------------------------------------------------------------
 */

#include "CC1101Driver.h"
#include "BoardConfig.h"

#include <SPI.h>
#include <Arduino.h>

/****************************************************************/
#define   WRITE_BURST       0x40            //write burst
#define   READ_SINGLE       0x80            //read single
#define   READ_BURST        0xC0            //read burst
#define   BYTES_IN_RXFIFO   0x7F            //byte number in RXfifo

byte modulation = 2;
int pa = 12;
byte last_pa;
byte gdo_set = 0;
bool spi = 0;
float MHz = 433.92;
byte m2DCOFF;
byte m2MANCH;
byte m2SYNCM;
byte m1FEC;
byte m1PRE;
byte m1CHSP;
byte pc1PQT;
byte pc1CRC_AF;
byte pc1APP_ST;
byte pc1ADRCHK;
byte pc0WDATA;
byte pc0PktForm;
byte pc0CRC_EN;
byte pc0LenConf;
byte clb1[2] =
{ 24, 28 };
byte clb2[2] =
{ 31, 38 };
byte clb3[2] =
{ 65, 76 };
byte clb4[2] =
{ 77, 79 };

/****************************************************************/
uint8_t PA_TABLE[8]
{ 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

CC1101Driver::CC1101Driver()
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
	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
}

CC1101Driver::~CC1101Driver()
{
	SPI.endTransaction();
	SPI.end();
}

/****************************************************************
 *FUNCTION NAME:Reset
 *FUNCTION     :CC1101 reset //details refer datasheet of CC1101/CC1100//
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::Reset(void)
{
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
	digitalWrite(CS0_PIN, HIGH);
}
/****************************************************************
 *FUNCTION NAME:Init
 *FUNCTION     :CC1101 initialization
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::Init(void)
{
	digitalWrite(CS0_PIN, HIGH);
	digitalWrite(SCK_PIN, HIGH);
	digitalWrite(MOSI_PIN, LOW);
	Reset();                    //CC1101 reset
	RegConfigSettings();            //CC1101 register config
}
/****************************************************************
 *FUNCTION NAME:SpiWriteReg
 *FUNCTION     :CC1101 write data to register
 *INPUT        :addr: register address; value: register value
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::SpiWriteReg(byte addr, byte value)
{
	digitalWrite(CS0_PIN, LOW);
	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(addr);
	SPI.transfer(value);
	digitalWrite(CS0_PIN, HIGH);
}
/****************************************************************
 *FUNCTION NAME:SpiWriteBurstReg
 *FUNCTION     :CC1101 write burst data to register
 *INPUT        :addr: register address; buffer:register value array; num:number to write
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::SpiWriteBurstReg(byte addr, byte *buffer, byte num)
{
	byte i, temp;

	temp = addr | WRITE_BURST;
	digitalWrite(CS0_PIN, LOW);
	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(temp);
	for (i = 0; i < num; i++)
	{
		SPI.transfer(buffer[i]);
	}
	digitalWrite(CS0_PIN, HIGH);
}

/****************************************************************
 *FUNCTION NAME:SpiStrobe
 *FUNCTION     :CC1101 Strobe
 *INPUT        :strobe: command; //refer define in CC1101.h//
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::SpiStrobe(byte strobe)
{
	digitalWrite(CS0_PIN, LOW);
	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(strobe);
	digitalWrite(CS0_PIN, HIGH);
}
/****************************************************************
 *FUNCTION NAME:SpiReadReg
 *FUNCTION     :CC1101 read data from register
 *INPUT        :addr: register address
 *OUTPUT       :register value
 ****************************************************************/
byte CC1101Driver::SpiReadReg(byte addr)
{
	byte temp, value;

	temp = addr | READ_SINGLE;
	digitalWrite(CS0_PIN, LOW);
	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(temp);
	value = SPI.transfer(0);
	digitalWrite(CS0_PIN, HIGH);

	return value;
}

/****************************************************************
 *FUNCTION NAME:SpiReadBurstReg
 *FUNCTION     :CC1101 read burst data from register
 *INPUT        :addr: register address; buffer:array to store register value; num: number to read
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::SpiReadBurstReg(byte addr, byte *buffer, byte num)
{
	byte i, temp;

	temp = addr | READ_BURST;
	digitalWrite(CS0_PIN, LOW);
	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(temp);
	for (i = 0; i < num; i++)
	{
		buffer[i] = SPI.transfer(0);
	}
	digitalWrite(CS0_PIN, HIGH);
}

/****************************************************************
 *FUNCTION NAME:SpiReadStatus
 *FUNCTION     :CC1101 read status register
 *INPUT        :addr: register address
 *OUTPUT       :status value
 ****************************************************************/
byte CC1101Driver::SpiReadStatus(byte addr)
{
	byte value, temp;

	temp = addr | READ_BURST;
	digitalWrite(CS0_PIN, LOW);
	while (digitalRead(MISO_PIN))
		;
	SPI.transfer(temp);
	value = SPI.transfer(0);
	digitalWrite(CS0_PIN, HIGH);

	return value;
}

/****************************************************************
 *FUNCTION NAME:Frequency Calculator
 *FUNCTION     :Calculate the basic frequency.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setMHZ(float mhz)
{
	byte freq2 = 0;
	byte freq1 = 0;
	byte freq0 = 0;

	MHz = mhz;

	for (bool i = 0; i == 0;)
	{
		if (mhz >= 26)
		{
			mhz -= 26;
			freq2 += 1;
		}
		else if (mhz >= 0.1015625)
		{
			mhz -= 0.1015625;
			freq1 += 1;
		}
		else if (mhz >= 0.00039675)
		{
			mhz -= 0.00039675;
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
/****************************************************************
 *FUNCTION NAME:Calibrate
 *FUNCTION     :Calibrate frequency
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::Calibrate(void)
{

	if (MHz >= 300 && MHz <= 348)
	{
		SpiWriteReg(CC1101_FSCTRL0, map(MHz, 300, 348, clb1[0], clb1[1]));
		if (MHz < 322.88)
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
	else if (MHz >= 378 && MHz <= 464)
	{
		SpiWriteReg(CC1101_FSCTRL0, map(MHz, 378, 464, clb2[0], clb2[1]));
		if (MHz < 430.5)
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
	else if (MHz >= 779 && MHz <= 899.99)
	{
		SpiWriteReg(CC1101_FSCTRL0, map(MHz, 779, 899, clb3[0], clb3[1]));
		if (MHz < 861)
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
	else if (MHz >= 900 && MHz <= 928)
	{
		SpiWriteReg(CC1101_FSCTRL0, map(MHz, 900, 928, clb4[0], clb4[1]));
		SpiWriteReg(CC1101_TEST0, 0x09);
		int s = SpiReadStatus(CC1101_FSCAL2);
		if (s < 32)
		{
			SpiWriteReg(CC1101_FSCAL2, s + 32);
		}
	}
}
/****************************************************************
 *FUNCTION NAME:Calibration offset
 *FUNCTION     :Set calibration offset
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setClb(byte b, byte s, byte e)
{
	if (b == 1)
	{
		clb1[0] = s;
		clb1[1] = e;
	}
	else if (b == 2)
	{
		clb2[0] = s;
		clb2[1] = e;
	}
	else if (b == 3)
	{
		clb3[0] = s;
		clb3[1] = e;
	}
	else if (b == 4)
	{
		clb4[0] = s;
		clb4[1] = e;
	}
}
/****************************************************************
 *FUNCTION NAME:getCC1101
 *FUNCTION     :Test Spi connection and return 1 when true.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
bool CC1101Driver::getCC1101(void)
{
	if (SpiReadStatus(0x31) > 0)
		return 1;
	else
		return 0;
}

/****************************************************************
 *FUNCTION NAME:Set Sync_Word
 *FUNCTION     :Sync Word
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setSyncWord(byte sh, byte sl)
{
	SpiWriteReg(CC1101_SYNC1, sh);
	SpiWriteReg(CC1101_SYNC0, sl);
}
/****************************************************************
 *FUNCTION NAME:Set ADDR
 *FUNCTION     :Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setAddr(byte v)
{
	SpiWriteReg(CC1101_ADDR, v);
}
/****************************************************************
 *FUNCTION NAME:Set PQT
 *FUNCTION     :Preamble quality estimator threshold
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setPQT(byte v)
{
	Split_PKTCTRL1();
	pc1PQT = 0;
	if (v > 7)
	{
		v = 7;
	}
	pc1PQT = v * 32;
	SpiWriteReg(CC1101_PKTCTRL1, pc1PQT + pc1CRC_AF + pc1APP_ST + pc1ADRCHK);
}
/****************************************************************
 *FUNCTION NAME:Set CRC_AUTOFLUSH
 *FUNCTION     :Enable automatic flush of RX FIFO when CRC is not OK
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setCRC_AF(bool v)
{
	Split_PKTCTRL1();
	pc1CRC_AF = 0;
	if (v == 1)
	{
		pc1CRC_AF = 8;
	}
	SpiWriteReg(CC1101_PKTCTRL1, pc1PQT + pc1CRC_AF + pc1APP_ST + pc1ADRCHK);
}
/****************************************************************
 *FUNCTION NAME:Set APPEND_STATUS
 *FUNCTION     :When enabled, two status bytes will be appended to the payload of the packet
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setAppendStatus(bool v)
{
	Split_PKTCTRL1();
	pc1APP_ST = 0;
	if (v == 1)
	{
		pc1APP_ST = 4;
	}
	SpiWriteReg(CC1101_PKTCTRL1, pc1PQT + pc1CRC_AF + pc1APP_ST + pc1ADRCHK);
}
/****************************************************************
 *FUNCTION NAME:Set ADR_CHK
 *FUNCTION     :Controls address check configuration of received packages
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setAdrChk(byte v)
{
	Split_PKTCTRL1();
	pc1ADRCHK = 0;
	if (v > 3)
	{
		v = 3;
	}
	pc1ADRCHK = v;
	SpiWriteReg(CC1101_PKTCTRL1, pc1PQT + pc1CRC_AF + pc1APP_ST + pc1ADRCHK);
}
/****************************************************************
 *FUNCTION NAME:Set WHITE_DATA
 *FUNCTION     :Turn data whitening on / off.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setWhiteData(bool v)
{
	Split_PKTCTRL0();
	pc0WDATA = 0;
	if (v == 1)
	{
		pc0WDATA = 64;
	}
	SpiWriteReg(CC1101_PKTCTRL0, pc0WDATA + pc0PktForm + pc0CRC_EN + pc0LenConf);
}
/****************************************************************
 *FUNCTION NAME:Set PKT_FORMAT
 *FUNCTION     :Format of RX and TX data
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setPktFormat(byte v)
{
	Split_PKTCTRL0();
	pc0PktForm = 0;
	if (v > 3)
	{
		v = 3;
	}
	pc0PktForm = v * 16;
	SpiWriteReg(CC1101_PKTCTRL0, pc0WDATA + pc0PktForm + pc0CRC_EN + pc0LenConf);
}
/****************************************************************
 *FUNCTION NAME:Set CRC
 *FUNCTION     :CRC calculation in TX and CRC check in RX
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setCrc(bool v)
{
	Split_PKTCTRL0();
	pc0CRC_EN = 0;
	if (v == 1)
	{
		pc0CRC_EN = 4;
	}
	SpiWriteReg(CC1101_PKTCTRL0, pc0WDATA + pc0PktForm + pc0CRC_EN + pc0LenConf);
}
/****************************************************************
 *FUNCTION NAME:Set LENGTH_CONFIG
 *FUNCTION     :Configure the packet length
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setLengthConfig(byte v)
{
	Split_PKTCTRL0();
	pc0LenConf = 0;
	if (v > 3)
	{
		v = 3;
	}
	pc0LenConf = v;
	SpiWriteReg(CC1101_PKTCTRL0, pc0WDATA + pc0PktForm + pc0CRC_EN + pc0LenConf);
}
/****************************************************************
 *FUNCTION NAME:Set PACKET_LENGTH
 *FUNCTION     :Indicates the packet length
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setPacketLength(byte v)
{
	SpiWriteReg(CC1101_PKTLEN, v);
}

void CC1101Driver::setBw(float bw)
{
	int s1 = 3;
	int s2 = 3;
	for (int i = 0; i < 3; i++)
	{
		if (bw > 101.5625)
		{
			bw /= 2;
			s1--;
		}
		else
		{
			i = 3;
		}
	}
	for (int i = 0; i < 3; i++)
	{
		if (bw > 58.1)
		{
			bw /= 1.25;
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
	SpiWriteReg(CC1101_MDMCFG4, s1 + s2 + MDMCFG4);
}

void CC1101Driver::setRate(float br)
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
	SpiWriteReg(CC1101_MDMCFG4, MDMCFG4 + m4DaRa);
	SpiWriteReg(CC1101_MDMCFG3, MDMCFG3);
}

/****************************************************************
 *FUNCTION NAME:Set Devitation
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setDeviation(float d)
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
/****************************************************************
 *FUNCTION NAME:Split PKTCTRL0
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::Split_PKTCTRL1(void)
{
	int calc = SpiReadStatus(7);
	pc1PQT = 0;
	pc1CRC_AF = 0;
	pc1APP_ST = 0;
	pc1ADRCHK = 0;
	for (bool i = 0; i == 0;)
	{
		if (calc >= 32)
		{
			calc -= 32;
			pc1PQT += 32;
		}
		else if (calc >= 8)
		{
			calc -= 8;
			pc1CRC_AF += 8;
		}
		else if (calc >= 4)
		{
			calc -= 4;
			pc1APP_ST += 4;
		}
		else
		{
			pc1ADRCHK = calc;
			i = 1;
		}
	}
}
/****************************************************************
 *FUNCTION NAME:Split PKTCTRL0
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::Split_PKTCTRL0(void)
{
	int calc = SpiReadStatus(8);
	pc0WDATA = 0;
	pc0PktForm = 0;
	pc0CRC_EN = 0;
	pc0LenConf = 0;
	for (bool i = 0; i == 0;)
	{
		if (calc >= 64)
		{
			calc -= 64;
			pc0WDATA += 64;
		}
		else if (calc >= 16)
		{
			calc -= 16;
			pc0PktForm += 16;
		}
		else if (calc >= 4)
		{
			calc -= 4;
			pc0CRC_EN += 4;
		}
		else
		{
			pc0LenConf = calc;
			i = 1;
		}
	}
}
/****************************************************************
 *FUNCTION NAME:Split MDMCFG1
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::Split_MDMCFG1(void)
{
	int calc = SpiReadStatus(19);
	m1FEC = 0;
	m1PRE = 0;
	m1CHSP = 0;
	for (bool i = 0; i == 0;)
	{
		if (calc >= 128)
		{
			calc -= 128;
			m1FEC += 128;
		}
		else if (calc >= 16)
		{
			calc -= 16;
			m1PRE += 16;
		}
		else
		{
			m1CHSP = calc;
			i = 1;
		}
	}
}

void CC1101Driver::setSyncMode(uint8_t mode)
{
	SpiWriteReg(CC1101_MDMCFG2, mode);
}

/****************************************************************
 *FUNCTION NAME:RegConfigSettings
 *FUNCTION     :CC1101 register config //details refer datasheet of CC1101/CC1100//
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::RegConfigSettings(void)
{
	SpiWriteReg(CC1101_FSCTRL1, 0x06);

	// CC Mode
	SpiWriteReg(CC1101_IOCFG2, 0x0B);
	SpiWriteReg(CC1101_IOCFG0, 0x06);
	SpiWriteReg(CC1101_PKTCTRL0, 0x05);
	SpiWriteReg(CC1101_MDMCFG3, 0xF8);

	// 2-FSK
	setSyncMode(2);
	SpiWriteReg(CC1101_FREND0, 0x10);

	setMHZ(MHz);

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
	SpiWriteReg(CC1101_PKTCTRL1, 0x04);
	SpiWriteReg(CC1101_ADDR, 0x00);
	SpiWriteReg(CC1101_PKTLEN, 0x00);

	SpiWriteBurstReg(CC1101_PATABLE, PA_TABLE, 8);
}

/****************************************************************
 *FUNCTION NAME:SetTx
 *FUNCTION     :set CC1101 send data
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::SetTx(void)
{
	SpiStrobe(CC1101_SIDLE);
	SpiStrobe(CC1101_STX);        //start send
}

/****************************************************************
 *FUNCTION NAME:SetRx
 *FUNCTION     :set CC1101 to receive state
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::SetRx(void)
{
	SpiStrobe(CC1101_SIDLE);
	SpiStrobe(CC1101_SRX);        //start receive
}

/****************************************************************
 *FUNCTION NAME:RSSI Level
 *FUNCTION     :Calculating the RSSI Level
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
int CC1101Driver::getRssi(void)
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

/****************************************************************
 *FUNCTION NAME:LQI Level
 *FUNCTION     :get Lqi state
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
byte CC1101Driver::getLqi(void)
{
	byte lqi;
	lqi = SpiReadStatus(CC1101_LQI);
	return lqi;
}

/****************************************************************
 *FUNCTION NAME:setSidle
 *FUNCTION     :set Rx / TX Off
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101Driver::setSidle(void)
{
	SpiStrobe(CC1101_SIDLE);
}
