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

#include "CC1101Driver.h"
#include "BoardConfig.h"

#include <Arduino.h>
#include <SPI.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

// CC1101 Standard registers
#define CC1101_IOCFG2   0x00 // GDO2 output pin configuration
#define CC1101_IOCFG1   0x01 // GDO1 output pin configuration
#define CC1101_IOCFG0   0x02 // GDO0 output pin configuration
#define CC1101_FIFOTHR  0x03 // RX FIFO and TX FIFO thresholds
#define CC1101_SYNC1    0x04 // Sync word, high INT8U
#define CC1101_SYNC0    0x05 // Sync word, low INT8U
#define CC1101_PKTLEN   0x06 // Packet length
#define CC1101_PKTCTRL1 0x07 // Packet automation control
#define CC1101_PKTCTRL0 0x08 // Packet automation control
#define CC1101_ADDR     0x09 // Device address
#define CC1101_CHANNR   0x0A // Channel number
#define CC1101_FSCTRL1  0x0B // Frequency synthesizer control
#define CC1101_FSCTRL0  0x0C // Frequency synthesizer control
#define CC1101_FREQ2    0x0D // Frequency control word, high INT8U
#define CC1101_FREQ1    0x0E // Frequency control word, middle INT8U
#define CC1101_FREQ0    0x0F // Frequency control word, low INT8U
#define CC1101_MDMCFG4  0x10 // Modem configuration
#define CC1101_MDMCFG3  0x11 // Modem configuration
#define CC1101_MDMCFG2  0x12 // Modem configuration
#define CC1101_MDMCFG1  0x13 // Modem configuration
#define CC1101_MDMCFG0  0x14 // Modem configuration
#define CC1101_DEVIATN  0x15 // Modem deviation setting
#define CC1101_MCSM2    0x16 // Main Radio Control State Machine configuration
#define CC1101_MCSM1    0x17 // Main Radio Control State Machine configuration
#define CC1101_MCSM0    0x18 // Main Radio Control State Machine configuration
#define CC1101_FOCCFG   0x19 // Frequency Offset Compensation configuration
#define CC1101_BSCFG    0x1A // Bit Synchronization configuration
#define CC1101_AGCCTRL2 0x1B // AGC control
#define CC1101_AGCCTRL1 0x1C // AGC control
#define CC1101_AGCCTRL0 0x1D // AGC control
#define CC1101_WOREVT1  0x1E // High INT8U Event 0 timeout
#define CC1101_WOREVT0  0x1F // Low INT8U Event 0 timeout
#define CC1101_WORCTRL  0x20 // Wake On Radio control
#define CC1101_FREND1   0x21 // Front end RX configuration
#define CC1101_FREND0   0x22 // Front end TX configuration
#define CC1101_FSCAL3   0x23 // Frequency synthesizer calibration
#define CC1101_FSCAL2   0x24 // Frequency synthesizer calibration
#define CC1101_FSCAL1   0x25 // Frequency synthesizer calibration
#define CC1101_FSCAL0   0x26 // Frequency synthesizer calibration
#define CC1101_RCCTRL1  0x27 // RC oscillator configuration
#define CC1101_RCCTRL0  0x28 // RC oscillator configuration
#define CC1101_FSTEST   0x29 // Frequency synthesizer calibration control
#define CC1101_PTEST    0x2A // Production test
#define CC1101_AGCTEST  0x2B // AGC test
#define CC1101_TEST2    0x2C // Various test settings
#define CC1101_TEST1    0x2D // Various test settings
#define CC1101_TEST0    0x2E // Various test settings

// CC1101 Strobe commands
#define CC1101_SRES    0x30 // Reset chip.
#define CC1101_SFSTXON 0x31 // Enable and calibrate frequency synthesizer (if MCSM0.FS_AUTOCAL=1).
#define CC1101_SXOFF   0x32 // Turn off crystal oscillator.
#define CC1101_SCAL    0x33 // Calibrate frequency synthesizer and turn it off
#define CC1101_SRX     0x34 // Enable RX. Perform calibration first if coming from IDLE and
#define CC1101_STX     0x35 // In IDLE state: Enable TX. Perform calibration first if
#define CC1101_SIDLE   0x36 // Exit RX / TX, turn off frequency synthesizer and exit
#define CC1101_SAFC    0x37 // Perform AFC adjustment of the frequency synthesizer
#define CC1101_SWOR    0x38 // Start automatic RX polling sequence (Wake-on-Radio)
#define CC1101_SPWD    0x39 // Enter power down mode when CSn goes high.
#define CC1101_SFRX    0x3A // Flush the RX FIFO buffer.
#define CC1101_SFTX    0x3B // Flush the TX FIFO buffer.
#define CC1101_SWORRST 0x3C // Reset real time clock.
#define CC1101_SNOP    0x3D // No operation. May be used to pad strobe commands to two

// CC1101 Status registers
#define CC1101_PARTNUM    0x30
#define CC1101_VERSION    0x31
#define CC1101_FREQEST    0x32
#define CC1101_LQI        0x33
#define CC1101_RSSI       0x34
#define CC1101_MARCSTATE  0x35
#define CC1101_WORTIME1   0x36
#define CC1101_WORTIME0   0x37
#define CC1101_PKTSTATUS  0x38
#define CC1101_VCO_VC_DAC 0x39
#define CC1101_TXBYTES    0x3A
#define CC1101_RXBYTES    0x3B

// CC1101 Register arrays (PA table, RX & TX FIFOs)
#define CC1101_PATABLE 0x3E
#define CC1101_TXFIFO  0x3F
#define CC1101_RXFIFO  0x3F

// CC1101 SPI access flags
#define WRITE_BURST 0x40
#define READ_SINGLE 0x80
#define READ_BURST  0xC0

// CC1101 packet length configurations
#define CC1101_PACKET_LENGTH_MODE_FIXED    0
#define CC1101_PACKET_LENGTH_MODE_VARIABLE 1
#define CC1101_PACKET_LENGTH_MODE_INFINITE 2

// Minimum time in microseconds for CC1101 to restart its XTAL when exiting power-down mode
#define XTAL_RESTART_TIME_US 400

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

const uint8_t CC1101Driver::PA_TABLE[8] = {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/*
 * Constructor of CC1101Driver
 * Initialize class attributes, SPI HW and pins
 */
CC1101Driver::CC1101Driver()
    : rfFreq_mHz(869.840), spiSettings(SPISettings(4000000, MSBFIRST, SPI_MODE0)), nextCSHigh(0), freqEstArrayIndex(0), currentFreqOff(0)
{
    memset(freqEstArray, 0, sizeof(freqEstArray));

    // Set SPI pins as per BoardConfig.h configuration
    SPI.setMOSI(MOSI_PIN);
    SPI.setMISO(MISO_PIN);
    SPI.setSCK(SCK_PIN);
    SPI.setCS(CS0_PIN);

    // Set pins to the right configuration
    pinMode(SCK_PIN, OUTPUT);
    pinMode(MOSI_PIN, OUTPUT);
    pinMode(MISO_PIN, INPUT);
    pinMode(CS0_PIN, OUTPUT);
    pinMode(GDO0_PIN, INPUT);

    // Pins startup state
    digitalWrite(CS0_PIN, HIGH);
    digitalWrite(SCK_PIN, HIGH);
    digitalWrite(MOSI_PIN, LOW);

    // Start SPI driver
    SPI.begin();
    // In  the context of MicronetToNMEA, only CC1101 driver is using SPI, so we can
    // transaction once for all here and end the transaction in the destructor
    // In case CC1101 would share the SPI bus with other ICs, beginTransaction should
    // be moved into each SPI access member to avoid race conditions.
    SPI.beginTransaction(spiSettings);
}

/*
 * Destructor of CC1101Driver
 * Release SPI bus and reset pin config
 */
CC1101Driver::~CC1101Driver()
{
    SPI.endTransaction();
    SPI.end();

    // Set all pins to input mode
    pinMode(SCK_PIN, INPUT);
    pinMode(MOSI_PIN, INPUT);
    pinMode(MISO_PIN, INPUT);
    pinMode(CS0_PIN, INPUT);
    pinMode(GDO0_PIN, INPUT);
}

/*
 * Reset CC1101 IC
 */
void CC1101Driver::Reset(void)
{
    // Apply datasheet's procedure (19.1.2)
    ChipSelect();
    delay(1);
    ChipDeselect(1);
    delay(1);
    ChipSelect();
    while (digitalRead(MISO_PIN))
        ;
    SPI.transfer(CC1101_SRES);
    while (digitalRead(MISO_PIN))
        ;
    ChipDeselect(8);
}

/*
 * Initialize CC1101
 */
void CC1101Driver::Init(void)
{
    // Set SPI pins initial state
    digitalWrite(CS0_PIN, HIGH);
    digitalWrite(SCK_PIN, HIGH);
    digitalWrite(MOSI_PIN, LOW);
    // Reset CC1101
    Reset();
    // Set base configuration
    SetBaseConfiguration();
}

/*
 * Write one CC1101 register
 *   IN addr -> register address
 *   IN value -> value to be written
 */
void CC1101Driver::SpiWriteReg(uint8_t addr, uint8_t value, uint32_t guardTime_us)
{
    ChipSelect();

    while (digitalRead(MISO_PIN))
        ;
    SPI.transfer(addr);
    SPI.transfer(value);

    ChipDeselect(guardTime_us);
}

/*
 * Write several consecutive CC1101 registers with a burst SPI access
 *   IN addr -> address of the first register
 *   IN buffer -> pointer to the array of bytes to be written
 *   IN num -> number of bytes to be written
 */
void CC1101Driver::SpiWriteBurstReg(uint8_t addr, uint8_t const *buffer, uint8_t nbBytes, uint32_t guardTime_us)
{
    ChipSelect();

    while (digitalRead(MISO_PIN))
        ;

    SPI.transfer(addr | WRITE_BURST);
    SPI.transfer(buffer, nullptr, nbBytes);

    ChipDeselect(guardTime_us);
}

/*
 * Send a strobe command to CC1101
 *   IN strobe -> strobe command byte
 */
void CC1101Driver::SpiStrobe(uint8_t strobe, uint32_t guardTime_us)
{
    ChipSelect();

    while (digitalRead(MISO_PIN))
        ;
    SPI.transfer(strobe);

    ChipDeselect(guardTime_us);
}

/*
 * Read one CC1101 register
 *   IN addr -> register address
 *   RETURN -> value of the register
 */
uint8_t CC1101Driver::SpiReadReg(uint8_t addr)
{
    ChipSelect();

    while (digitalRead(MISO_PIN))
        ;
    SPI.transfer(addr | READ_SINGLE);
    uint8_t value = SPI.transfer(0);

    ChipDeselect(0);

    return value;
}

/*
 * Read several consecutive CC1101 registers with a burst SPI access
 *   IN addr -> address of the first register
 *   OUT buffer -> pointer to the array of bytes where to store read data
 *   IN num -> number of bytes to be read
 */
void CC1101Driver::SpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t nbBytes)
{
    ChipSelect();

    while (digitalRead(MISO_PIN))
        ;
    SPI.transfer(addr | READ_BURST);
    SPI.transfer(nullptr, buffer, nbBytes);

    ChipDeselect(0);
}

/*
 * Read CC1101 chip status byte
 */
uint8_t CC1101Driver::SpiReadChipStatusByte()
{
    ChipSelect();

    while (digitalRead(MISO_PIN))
        ;
    uint8_t value = SPI.transfer(CC1101_SNOP);

    ChipDeselect(0);

    return value;
}

/*
 * Read one CC1101 status register
 *   IN addr -> register address
 *   RETURN -> value of the register
 */
uint8_t CC1101Driver::SpiReadStatus(uint8_t addr)
{
    ChipSelect();

    while (digitalRead(MISO_PIN))
        ;
    SPI.transfer(addr | READ_BURST);
    uint8_t value = SPI.transfer(0);

    ChipDeselect(0);

    return value;
}

/*
 * Set the frequency of CC1101 down converter
 *   IN frq_MHz -> Frequency in MHz
 */
void CC1101Driver::SetFrequency(float freq_Mhz)
{
    uint8_t freq2 = 0;
    uint8_t freq1 = 0;
    uint8_t freq0 = 0;

    rfFreq_mHz = freq_Mhz;

    // TODO : rewrite with a faster algorithm
    for (bool i = 0; i == 0;)
    {
        if (freq_Mhz >= 26)
        {
            freq_Mhz -= 26;
            freq2 += 1;
        }
        else if (freq_Mhz >= 0.1015625)
        {
            freq_Mhz -= 0.1015625;
            freq1 += 1;
        }
        else if (freq_Mhz >= 0.00039675)
        {
            freq_Mhz -= 0.00039675;
            freq0 += 1;
        }
        else
        {
            i = 1;
        }
    }
    if (freq0 > 255)
    {
        // TODO : bug, freq1 can also overflow
        freq1 += 1;
        freq0 -= 256;
    }

    SpiWriteReg(CC1101_FREQ2, freq2, 8);
    SpiWriteReg(CC1101_FREQ1, freq1, 8);
    SpiWriteReg(CC1101_FREQ0, freq0, 8);

    // Launch PLL calibration
    Calibrate();
}

/*
 * Calibrate CC1101's PLL
 */
void CC1101Driver::Calibrate(void)
{
    currentFreqOff = 0;

    // We consider here that frequencies can not be others than the ones
    // used by Micronet : 869.840 or 915.915 Mhz. Don't use this driver
    // for other frequencies
    SpiWriteReg(CC1101_TEST0, 0x09, 8);
    SpiWriteReg(CC1101_FSCTRL0, currentFreqOff, 8);
    // Trigger calibration
    SpiStrobe(CC1101_SCAL, 8);
    // Wait for calibration to end. Can last 700-800us
    while ((SpiReadChipStatusByte() & 0x40) != 0)
        ;
}

/*
 * Checks if CC1101 is present on SPI bus
 */
bool CC1101Driver::IsConnected(void)
{
    if (SpiReadStatus(0x31) > 0)
        return 1;
    else
        return 0;
}

/*
 * Set sync word of packet handler
 */
void CC1101Driver::SetSyncWord(uint8_t msb, uint8_t lsb)
{
    SpiWriteReg(CC1101_SYNC1, msb, 8);
    SpiWriteReg(CC1101_SYNC0, lsb, 8);
}

/*
 * Set Preamble Quality Threshold
 *   IN pqt -> quality threshold
 */
void CC1101Driver::SetPQT(uint8_t pqt)
{
    SpiWriteReg(CC1101_PKTCTRL1, pqt << 5, 8);
}

/*
 * Set packet handler length configuration
 *   IN config -> 0 : Fixed packet length
 *                1 : Variable packet length
 *                2 : infinite packet length
 */
void CC1101Driver::SetLengthConfig(uint8_t config)
{
    uint8_t PKTCTRL0 = SpiReadReg(CC1101_PKTCTRL0) & 0xfc;
    SpiWriteReg(CC1101_PKTCTRL0, PKTCTRL0 | config, 8);
}

/*
 * Set packet length when in fixed packet mode
 *   IN length -> packet length in bytes
 */
void CC1101Driver::SetPacketLength(uint8_t length)
{
    SpiWriteReg(CC1101_PKTLEN, length, 8);
}

/*
 * Get the current RX FIFO level
 *   RETURN level of RX the FIFO in bytes
 */
int CC1101Driver::GetRxFifoLevel()
{
    return SpiReadStatus(CC1101_RXBYTES);
}

/*
 * Get the current RX FIFO level
 *   RETURN level of RX the FIFO in bytes
 */
int CC1101Driver::GetTxFifoLevel()
{
    return SpiReadStatus(CC1101_TXBYTES);
}

/*
 * Read data from RX FIFO
 *   OUT buffer -> pointer to the buffer to write FIFO data to
 *   IN nbBytes -> number of bytes to be read
 */
void CC1101Driver::ReadRxFifo(uint8_t *buffer, int nbBytes)
{
    SpiReadBurstReg(CC1101_RXFIFO, buffer, nbBytes);
}

/*
 * Write one byte to TX FIFO
 *   IN data -> byte to be written
 */
void CC1101Driver::WriteTxFifo(uint8_t data)
{
    SpiWriteReg(CC1101_TXFIFO, data, 8);
}

/*
 * Write multiple bytes of data to TX FIFO
 *   IN buffer -> pointer to the buffer with data to be sent
 *   IN nbBytes -> number of bytes to be written
 */
void CC1101Driver::WriteArrayTxFifo(uint8_t const *buffer, int nbBytes)
{
    SpiWriteBurstReg(CC1101_TXFIFO, buffer, nbBytes, 8);
}

/*
 * Configure CC1101 to trigger GDO0 IRQ when TX FIFO underflows
 */
void CC1101Driver::IrqOnTxFifoUnderflow()
{
    SpiWriteReg(CC1101_IOCFG0, 0x05, 8);
}

/*
 * Configure CC1101 to trigger GDO0 IRQ when TX FIFO is below FIFO threshold
 */
void CC1101Driver::IrqOnTxFifoThreshold()
{
    SpiWriteReg(CC1101_IOCFG0, 0x42, 8);
}

/*
 * Configure CC1101 to trigger GDO0 IRQ when RX FIFO is above FIFO threshold
 * or when packet has been entirely received
 */
void CC1101Driver::IrqOnRxFifoThreshold()
{
    SpiWriteReg(CC1101_IOCFG0, 0x01, 8);
}

/*
 * Configure RX/TX FIFO threshold
 */
void CC1101Driver::SetFifoThreshold(uint8_t fifoThreshold)
{
    SpiWriteReg(CC1101_FIFOTHR, fifoThreshold, 8);
}

/*
 * Flush RX FIFO
 */
void CC1101Driver::FlushRxFifo()
{
    SpiStrobe(CC1101_SFRX, 8);
}

/*
 * Flush TX FIFO
 */
void CC1101Driver::FlushTxFifo()
{
    SpiStrobe(CC1101_SFTX, 8);
}

/*
 * Set down converter bandwidth
 *   IN bw_kHz -> Bandwidth in kHz
 */
void CC1101Driver::SetBw(float bw_kHz)
{
    int s1 = 3;
    int s2 = 3;

    // TODO : check algorithm against datasheet.
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
    SpiWriteReg(CC1101_MDMCFG4, MDMCFG4 | s1 | s2, 8);
}

/*
 * Sets demodulator data rate
 *   IN baudrate -> baudrate in baud
 */
void CC1101Driver::SetBitrate(float bitrate)
{
    uint8_t m4DaRa;
    float   c       = bitrate;
    uint8_t MDMCFG3 = 0;

    // TODO : verify algorithm
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
            c        = c - 0.0247955;
            c        = c / 0.00009685;
            MDMCFG3  = c;
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
    SpiWriteReg(CC1101_MDMCFG4, MDMCFG4 | m4DaRa, 8);
    SpiWriteReg(CC1101_MDMCFG3, MDMCFG3, 8);
}

/*
 * Set demodulator's 2-FSK deviation
 *   IN deviation_kHz -> deviation in kHz
 */
void CC1101Driver::SetDeviation(float deviation_kHz)
{
    float f = 1.586914;
    float v = 0.19836425;
    int   c = 0;

    // TODO : Check algorithm
    if (deviation_kHz > 380.859375)
    {
        deviation_kHz = 380.859375;
    }
    if (deviation_kHz < 1.586914)
    {
        deviation_kHz = 1.586914;
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
        if (f >= deviation_kHz)
        {
            c = i;
            i = 255;
        }
        c++;
    }
    SpiWriteReg(CC1101_DEVIATN, c, 8);
}

/*
 * Set Sync Word detection qualifier word
 *   IN mode -> sync mode as per MDMCFG2 definition in datasheet
 */
void CC1101Driver::SetSyncMode(uint8_t mode)
{
    // TODO : Add enum for mode
    SpiWriteReg(CC1101_MDMCFG2, mode, 8);
}

/*
 * Switch CC1101 to TX mode
 * CC1101 start sending data in the TX FIFO after this call
 */
void CC1101Driver::SetTx(void)
{
    SpiStrobe(CC1101_SIDLE, 8);
    SpiStrobe(CC1101_STX, 8);
}

/*
 * Switch CC1101 to RX mode
 * CC1101 start listening for incoming preamble/sync word after this call
 */
void CC1101Driver::SetRx(void)
{
    SpiStrobe(CC1101_SIDLE, 8);
    SpiStrobe(CC1101_SRX, 8);
}

/*
 * Switch CC1101 to idle mode
 * CC1101 stops receiving or transmitting data after this call
 */
void CC1101Driver::SetSidle(void)
{
    SpiStrobe(CC1101_SIDLE, 8);
}

/*
 * Switch CC1101 to low power mode
 * Disables XTAL input to reduce power consumption
 */
void CC1101Driver::LowPower()
{
    SpiStrobe(CC1101_SIDLE, 8);
    SpiStrobe(CC1101_SXOFF, 8);
}

/*
 * Switch CC1101 to active mode
 * this function must be called to exit low power mode and restore
 * XTAL input. be careful that it can take up to 800-900us to
 * complete
 */
void CC1101Driver::ActivePower()
{
    uint32_t timeRef, timeLimit;

    // Force exit of power-down mode
    SpiStrobe(CC1101_SIDLE, 8);

    // Let time to CC1101 to restart its XTAL
    timeRef   = micros();
    timeLimit = timeRef + XTAL_RESTART_TIME_US;
    while (micros() < timeLimit)
    {
        if (micros() < timeRef)
            break;
    }

    // Trigger PLL calibration
    SpiStrobe(CC1101_SCAL, 8);
}

/*
 * Get RSSI of latest packet reception
 * RETURN RSSI value in dbm
 */
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

/*
 * Get Line Quality Indicator of latest packet reception
 * RETURN LQI value
 */
uint8_t CC1101Driver::GetLqi(void)
{
    uint8_t lqi;
    lqi = SpiReadStatus(CC1101_LQI);
    return lqi;
}

/*
 * Define the base configuration of CC1101
 * Note that this driver is dedicated to Micronet RF talking so its
 * configuration is entirely specific to Micronet.
 */
void CC1101Driver::SetBaseConfiguration(void)
{
    SpiWriteReg(CC1101_FSCTRL1, 0x08, 8); // IF frequency

    // CC Mode
    SpiWriteReg(CC1101_IOCFG2, 0x6F, 8);   // GDO2 unused
    SpiWriteReg(CC1101_IOCFG1, 0x6F, 8);   // GDO1 unused
    SpiWriteReg(CC1101_IOCFG0, 0x01, 8);   // GDO0 on RX FIFO by default
    SpiWriteReg(CC1101_PKTCTRL0, 0x02, 8); // Infinite packet length by default
    SpiWriteReg(CC1101_MDMCFG3, 0xF8, 8);  // Default bitrate

    // 2-FSK
    SetSyncMode(2);                      // 16/16 Sync mode
    SpiWriteReg(CC1101_FREND0, 0x10, 8); // Value given by SmartRF studio

    // RF Frequency
    SetFrequency(rfFreq_mHz); // Set down converter frequency

    SpiWriteReg(CC1101_MDMCFG1, 0x02, 8);  // Channel spacing (unused with Micronet)
    SpiWriteReg(CC1101_MDMCFG0, 0xF8, 8);  // Channel spacing (unused with Micronet)
    SpiWriteReg(CC1101_CHANNR, 0x00, 8);   // We don't use channels : set to 0
    SpiWriteReg(CC1101_DEVIATN, 0x47, 8);  // Default deviation. Will be overwritten later at init
    SpiWriteReg(CC1101_FREND1, 0x56, 8);   // Front-end : value given by Smart RF studio
    SpiWriteReg(CC1101_MCSM0, 0x08, 8);    // PO_TIMEOUT = 2 -> ~150us for XOSC to stabilize
    SpiWriteReg(CC1101_FOCCFG, 0x16, 8);   // Frequency offset algorithm coefficients
    SpiWriteReg(CC1101_BSCFG, 0x1C, 8);    // No bitrate compensation
    SpiWriteReg(CC1101_AGCCTRL2, 0xC7, 8); // AGC : value from SmartRF studio
    SpiWriteReg(CC1101_AGCCTRL1, 0x00, 8); // AGC : value from SmartRF studio
    SpiWriteReg(CC1101_AGCCTRL0, 0xB2, 8); // AGC : value from SmartRF studio
    SpiWriteReg(CC1101_FSCAL3, 0xE9, 8);   // Value from SMartRF studio
    SpiWriteReg(CC1101_FSCAL2, 0x2A, 8);   // Value from SMartRF studio
    SpiWriteReg(CC1101_FSCAL1, 0x00, 8);   // Value from SMartRF studio
    SpiWriteReg(CC1101_FSCAL0, 0x1F, 8);   // Value from SMartRF studio
    SpiWriteReg(CC1101_FSTEST, 0x59, 8);   // ?
    SpiWriteReg(CC1101_TEST2, 0x81, 8);    // Value from SMartRF studio
    SpiWriteReg(CC1101_TEST1, 0x35, 8);    // Value from SMartRF studio
    SpiWriteReg(CC1101_TEST0, 0x09, 8);    // Value from SMartRF studio
    SpiWriteReg(CC1101_PKTCTRL1, 0x80, 8); // PQT = 4, packet handler disabled
    SpiWriteReg(CC1101_ADDR, 0x00, 8);     // No address handling
    SpiWriteReg(CC1101_PKTLEN, 0x00, 8);   // No packet length

    // Full power in TX mode (12dbm@869MHz 11dbm@915MHz)
    SpiWriteBurstReg(CC1101_PATABLE, PA_TABLE, 8, 8);
}

/*
 * Assert CC1101 CS line
 */
void CC1101Driver::ChipSelect()
{
    // Before asserting CS, we check that will let enough time to
    // CC1101 to process the previous SPI command. If necessary, we
    // wait the required amount of time.
    while (micros() < nextCSHigh)
    {
        if (nextCSHigh - micros() > 10)
        {
            // If we have more 10us to wait, it means micros' counter looped on its maximum value
            break;
        }
    }

    digitalWrite(CS0_PIN, LOW);
}

/*
 * Release CC1101 CS line
 */
void CC1101Driver::ChipDeselect(uint32_t guardTime_us)
{
    digitalWrite(CS0_PIN, HIGH);
    // We store the time at which we release CS to allow the next CS
    // assertion to ensure enough time has been let to CC1101 to
    // process the command.
    nextCSHigh = micros() + guardTime_us;
}

/*
 * Update CC1101 frequency offset compensation by checking
 * the result of the internal frequency estimator
 */
void CC1101Driver::UpdateFreqOffset()
{
    int8_t freqEst;

    // Read latest frequency offset estimation and store it in the averaging array
    freqEst                           = SpiReadStatus(CC1101_FREQEST);
    freqEstArray[freqEstArrayIndex++] = freqEst;

    // Only calculate new offset when averaging array is full
    if (freqEstArrayIndex >= FREQ_ESTIMATION_ARRAY_SIZE)
    {
        int32_t avgFreqEst = 0;
        freqEstArrayIndex  = 0;
        // Calculate average frequency offset
        for (int i = 0; i < FREQ_ESTIMATION_ARRAY_SIZE; i++)
        {
            avgFreqEst += freqEstArray[i];
        }
        avgFreqEst /= FREQ_ESTIMATION_ARRAY_SIZE;

        // Clip value to 8 bit
        int32_t newFreqOff = currentFreqOff + avgFreqEst;
        if (newFreqOff >= 127)
            newFreqOff = 127;
        if (newFreqOff <= -128)
            newFreqOff = -128;
        // Update offset
        currentFreqOff = newFreqOff;
        SpiWriteReg(CC1101_FSCTRL0, currentFreqOff, 8);
    }
}
