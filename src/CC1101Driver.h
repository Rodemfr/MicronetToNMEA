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

#ifndef CC1101DRIVER_H_
#define CC1101DRIVER_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include <Arduino.h>
#include <SPI.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* Number of samples that will be used to evaluate the frequency deviation
 * between MicronetToNMEA and Micronet's master device
 * A value of 32 means that frequency deviation will be averaged on 32 seconds
 * before applying a correction */
#define FREQ_ESTIMATION_ARRAY_SIZE 32

// CC1101 Constants to define RX FIFO thresholds
#define CC1101_RXFIFOTHR_4  0x00
#define CC1101_RXFIFOTHR_8  0x01
#define CC1101_RXFIFOTHR_12 0x02
#define CC1101_RXFIFOTHR_16 0x03
#define CC1101_RXFIFOTHR_20 0x04
#define CC1101_RXFIFOTHR_24 0x05
#define CC1101_RXFIFOTHR_28 0x06
#define CC1101_RXFIFOTHR_32 0x07
#define CC1101_RXFIFOTHR_36 0x08
#define CC1101_RXFIFOTHR_40 0x09
#define CC1101_RXFIFOTHR_44 0x0a
#define CC1101_RXFIFOTHR_48 0x0b
#define CC1101_RXFIFOTHR_52 0x0c
#define CC1101_RXFIFOTHR_56 0x0d
#define CC1101_RXFIFOTHR_60 0x0e
#define CC1101_RXFIFOTHR_64 0x0f

// CC1101 Constants to define TX FIFO thresholds
#define CC1101_TXFIFOTHR_61 0x00
#define CC1101_TXFIFOTHR_57 0x01
#define CC1101_TXFIFOTHR_53 0x02
#define CC1101_TXFIFOTHR_49 0x03
#define CC1101_TXFIFOTHR_45 0x04
#define CC1101_TXFIFOTHR_41 0x05
#define CC1101_TXFIFOTHR_37 0x06
#define CC1101_TXFIFOTHR_33 0x07
#define CC1101_TXFIFOTHR_29 0x08
#define CC1101_TXFIFOTHR_25 0x09
#define CC1101_TXFIFOTHR_21 0x0a
#define CC1101_TXFIFOTHR_17 0x0b
#define CC1101_TXFIFOTHR_13 0x0c
#define CC1101_TXFIFOTHR_9  0x0d
#define CC1101_TXFIFOTHR_5  0x0e
#define CC1101_TXFIFOTHR_1  0x0f

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class CC1101Driver
{
public:
	CC1101Driver();
	~CC1101Driver();

	void Init(void);
	void SetFrequency(float freq_mhz);
	void SetSyncMode(uint8_t mode);
	void SetBw(float bw);
	void SetRate(float br);
	void SetDeviation(float d);
	void SetTx(void);
	void SetRx(void);
	int GetRssi(void);
	uint8_t GetLqi(void);
	void SetSidle(void);
	void LowPower();
	void ActivePower();
	bool IsConnected(void);
	void SetSyncWord(uint8_t sh, uint8_t sl);
	void SetPQT(uint8_t pqt);
	void SetLengthConfig(uint8_t v);
	void SetPacketLength(uint8_t v);
	int GetRxFifoLevel();
	int GetTxFifoLevel();
	void ReadRxFifo(uint8_t *buffer, int nbBytes);
	void WriteTxFifo(uint8_t data);
	void WriteArrayTxFifo(uint8_t const *buffer, int nbBytes);
	void IrqOnTxFifoUnderflow();
	void IrqOnTxFifoThreshold();
	void IrqOnRxFifoThreshold();
	void SetFifoThreshold(uint8_t fifoThreshold);
	void FlushRxFifo();
	void FlushTxFifo();
	void UpdateFreqOffset();

private:
	float rfFreq_mHz;
	SPISettings spiSettings;
	uint32_t lastCSHigh;
	int freqEstArrayIndex;
	int8_t freqEstArray[FREQ_ESTIMATION_ARRAY_SIZE];
	int8_t currentFreqOff;
	static const uint8_t PA_TABLE[8];

	void Reset(void);
	void SetBaseConfiguration(void);
	void Calibrate(void);
	uint8_t SpiReadChipStatusByte();
	uint8_t SpiReadStatus(uint8_t addr);
	void SpiStrobe(uint8_t strobe);
	void SpiWriteReg(uint8_t addr, uint8_t value);
	void SpiWriteBurstReg(uint8_t addr, uint8_t const *buffer, uint8_t num);
	uint8_t SpiReadReg(uint8_t addr);
	void SpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t num);
	void ChipSelect();
	void ChipDeselect();
};

#endif
