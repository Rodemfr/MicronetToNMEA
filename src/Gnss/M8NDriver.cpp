/***************************************************************************
 *                                                                         
 * M8NDriver.cpp                                                           
 *                                                                         
 * This module implements a driver for UBlox M6N/M8N GNSS receivers.
 * It configures the receiver through UBX binary protocol and PUBX messages
 * to:
 * - Set baudrate to 38400
 * - Configure GNSS constellations (GPS, GLONASS, Galileo)
 * - Enable/disable specific NMEA sentences
 * - Set navigation rate (1Hz or 5Hz)
 *                                                                         
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

#include "M8NDriver.h"
#include "BoardConfig.h"

#include <Arduino.h>
#include <cstdint>

/*
 * Binary UBX configuration messages
 * These messages are used to configure the receiver through UBX binary protocol
 */
const PROGMEM uint8_t M8NDriver::ClearConfig[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
                                                  0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x19, 0x98};
const PROGMEM uint8_t M8NDriver::UART1_38400[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00,
                                                  0x00, 0x96, 0x00, 0x00, 0x07, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x92, 0x8A};
// const PROGMEM uint8_t UART1115000[] = {0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x00, 0xC2, 0x01, 0x00,
// 0x23, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDB, 0x58}; const PROGMEM uint8_t Navrate1hz[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03,
// 0x01, 0x00, 0x01, 0x00, 0x01, 0x39}; const PROGMEM uint8_t Navrate2hz[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xF4, 0x01, 0x01, 0x00, 0x01, 0x00,
// 0x0B, 0x77};
const PROGMEM uint8_t M8NDriver::Navrate5hz[] = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A};
const PROGMEM uint8_t M8NDriver::GNSSSetup[]  = {0xB5, 0x62, 0x06, 0x3E, 0x3C, 0x00, 0x00, 0x00, 0x20, 0x07, 0x00, 0x08, 0x10, 0x00, 0x01, 0x00, 0x01,
                                                 0x01, 0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01,
                                                 0x03, 0x08, 0x10, 0x00, 0x00, 0x00, 0x01, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x01, 0x05,
                                                 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, 0x06, 0x08, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x01, 0x30, 0xAD};

/*
 * PUBX configuration messages
 * These messages enable/disable specific NMEA sentences output by the receiver
 */
const PROGMEM char DTM_off[] = "$PUBX,40,DTM,0,0,0,0,0,0*46";
const PROGMEM char GBS_off[] = "$PUBX,40,GBS,0,0,0,0,0,0*4D";
const PROGMEM char GGA_on[]  = "$PUBX,40,GGA,0,1,0,0,0,0*5B"; // GGA on
const PROGMEM char GGA_off[] = "$PUBX,40,GGA,0,0,0,0,0,0*5A"; // GGA off
const PROGMEM char GLL_off[] = "$PUBX,40,GLL,0,0,0,0,0,0*5C";
const PROGMEM char GNS_off[] = "$PUBX,40,GNS,0,0,0,0,0,0*41";
const PROGMEM char GRS_off[] = "$PUBX,40,GRS,0,0,0,0,0,0*5D";
const PROGMEM char GSA_off[] = "$PUBX,40,GSA,0,0,0,0,0,0*4E";
const PROGMEM char GST_off[] = "$PUBX,40,GST,0,0,0,0,0,0*5B";
const PROGMEM char GSV_off[] = "$PUBX,40,GSV,0,0,0,0,0,0*59";
const PROGMEM char RMC_on[]  = "$PUBX,40,RMC,0,1,0,0,0,0*46"; // RMC on
const PROGMEM char RMC_off[] = "$PUBX,40,RMC,0,0,0,0,0,0*47"; // RMC off
const PROGMEM char VLW_off[] = "$PUBX,40,VLW,0,0,0,0,0,0*56";
const PROGMEM char VTG_on[]  = "$PUBX,40,VTG,0,1,0,0,0,0*5F"; // VTG on
const PROGMEM char VTG_off[] = "$PUBX,40,VTG,0,0,0,0,0,0*5E"; // VTG off
const PROGMEM char THS_off[] = "$PUBX,40,THS,0,0,0,0,0,0*54";
const PROGMEM char ZDA_off[] = "$PUBX,40,ZDA,0,0,0,0,0,0*44";

/**
 * Default constructor
 */
M8NDriver::M8NDriver()
{
}

/**
 * Default destructor
 */
M8NDriver::~M8NDriver()
{
}

/**
 * Send a UBX binary configuration message to the receiver
 * 
 * @param progmemPtr Pointer to configuration message in program memory
 * @param arraySize Size of the configuration message
 */
void M8NDriver::GPS_SendConfig(const uint8_t *progmemPtr, uint8_t arraySize)
{
    uint8_t byteread, index;
    for (index = 0; index < arraySize; index++)
    {
        byteread = pgm_read_byte_near(progmemPtr++);
        if (byteread < 0x10)
        {
        }
    }
    progmemPtr = progmemPtr - arraySize;

    for (index = 0; index < arraySize; index++)
    {
        byteread = pgm_read_byte_near(progmemPtr++);
        NMEA0183_IN.write(byteread);
    }
    delay(100);
}

/**
 * Send a PUBX configuration message to the receiver
 * 
 * @param pubxMsg PUBX message to send (null terminated string)
 */
void M8NDriver::GPS_SendPUBX(const char pubxMsg[])
{
    NMEA0183_IN.println(pubxMsg);
}

/**
 * Initialize and configure the GNSS receiver
 * 
 * @param nmeaSentences Bit field indicating which NMEA sentences to enable:
 *                      - M8N_GGA_ENABLE: Enable GGA messages
 *                      - M8N_RMC_ENABLE: Enable RMC messages
 *                      - M8N_VTG_ENABLE: Enable VTG messages
 *                      - M8N_HISPEED_NAV: Set navigation rate to 5Hz
 */
void M8NDriver::Start(uint32_t nmeaSentences)
{
    GPS_SendConfig(ClearConfig, 21);
    delay(500);
    GPS_SendConfig(UART1_38400, 28);
    NMEA0183_IN.end();
    NMEA0183_IN.begin(38400);
    while (!NMEA0183_IN)
    {
        delay(10);
    }
    NMEA0183_IN.println("");
    GPS_SendConfig(GNSSSetup, 68);
    delay(200);
    NMEA0183_IN.println("");
    GPS_SendPUBX(DTM_off);
    GPS_SendPUBX(GBS_off);
    if (nmeaSentences & M8N_GGA_ENABLE)
        GPS_SendPUBX(GGA_on);
    else
        GPS_SendPUBX(GGA_off);

    GPS_SendPUBX(GLL_off);
    GPS_SendPUBX(GNS_off);
    GPS_SendPUBX(GRS_off);
    GPS_SendPUBX(GSA_off);
    GPS_SendPUBX(GST_off);
    GPS_SendPUBX(GSV_off);
    if (nmeaSentences & M8N_RMC_ENABLE)
        GPS_SendPUBX(RMC_on);
    else
        GPS_SendPUBX(RMC_off);

    GPS_SendPUBX(VLW_off);
    if (nmeaSentences & M8N_VTG_ENABLE)
        GPS_SendPUBX(VTG_on);
    else
        GPS_SendPUBX(VTG_off);
    // GPS_SendPUBX(THS_off);
    GPS_SendPUBX(ZDA_off);
    if (nmeaSentences & M8N_HISPEED_NAV)
        GPS_SendConfig(Navrate5hz, 14);
}
