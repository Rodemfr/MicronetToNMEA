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

#ifndef BOARDCONFIG_H_
#define BOARDCONFIG_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

// Select EU (868MHz) or not-EU (915MHz) Micronet frequency
// 0 -> EU (868Mhz)
// 1 -> non-EU (915Mhz)
#define FREQUENCY_SYSTEM 0

// Selects on which I2C bus is connected compass as per Wiring library definition
#if defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
#define NAVCOMPASS_I2C Wire1 // SDA1: 38 SCL1: 37
#else // Teensy 4.0 Configuration
#define NAVCOMPASS_I2C Wire // SDA0: 18 SCL0: 19
#endif

// CC1101/SPI pins
#if defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
#define CS0_PIN  10
#define MOSI_PIN 11
#define MISO_PIN 12
#define SCK_PIN  14
#define GDO0_PIN 24
#else // Teensy 4.0 Configuration
#define CS0_PIN  10
#define MOSI_PIN 11
#define MISO_PIN 12
#define SCK_PIN  13
#define GDO0_PIN 9
#endif

// NMEA GNSS UART pins
#if defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
#define GNSS_UBLOXM8N  1       // Set to one if your GNSS is a UBLOX M8N, 0 else. If set to one, GNSS will be automatically configured at startup
#define GNSS_SERIAL    Serial1
#define GNSS_BAUDRATE  9600
#define GNSS_RX_PIN    0
#define GNSS_TX_PIN    1
#else // Teensy 4.0 Configuration
#define GNSS_UBLOXM8N  1       // Set to one if your GNSS is a UBLOX M8N, 0 else. If set to one, GNSS will be automatically configured at startup
#define GNSS_SERIAL    Serial2
#define GNSS_BAUDRATE  9600
#define GNSS_RX_PIN    7
#define GNSS_TX_PIN    8
#endif

// USB UART params
#define USB_NMEA     SerialUSB
#define USB_BAUDRATE 115200

// Wired UART params
#if defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
#define WIRED_NMEA     Serial5
#define WIRED_BAUDRATE 115200
#define WIRED_RX_PIN   34
#define WIRED_TX_PIN   33
#else // Teensy 4.0 Configuration
#define WIRED_NMEA     Serial1
#define WIRED_BAUDRATE 115200
#define WIRED_RX_PIN   0
#define WIRED_TX_PIN   1
#endif

// The console to use for menu and NMEA output
#define CONSOLE  USB_NMEA
#define NMEA_EXT USB_NMEA

// Defines with data comes from which link
// LINK_NMEA_EXT -> data comes from external NMEA link (WIRED_NMEA)
// LINK_NMEA_GNSS -> data comes from GNSS NMEA link (GNSS_SERIAL)
// LINK_MICRONET -> data comes from Micronet network
// LINK_COMPASS -> data comes from LSM303 (NAVCOMPASS_I2C)
#define NAV_SOURCE_LINK     LINK_NMEA_EXT  // Navigation data (RMB)
#define GNSS_SOURCE_LINK    LINK_NMEA_GNSS // Positionning data (RMC, GGA, VTG)
#define WIND_SOURCE_LINK    LINK_MICRONET  // Wind data (MWV)
#define DEPTH_SOURCE_LINK   LINK_MICRONET  // Depth data (DPT)
#define SPEED_SOURCE_LINK   LINK_MICRONET  // Speed data (VHW, VLW)
#define VOLTAGE_SOURCE_LINK LINK_MICRONET  // Battery voltage data (XDR)
#define SEATEMP_SOURCE_LINK LINK_MICRONET  // Temperature data (STP)
#define COMPASS_SOURCE_LINK LINK_COMPASS   // Heading data (MTW)

// Navigation softwares can send a wrong RMB sentence and invert "FROM" and "TO" fields
// If you see your Micronet display showing the "FROM" waypoint name instead of the "TO"
// on the DTW & BTW pages, then change the following configuration key to 1
#define INVERTED_RMB_WORKAROUND 0

// In case your displays would have difficulties to receive data from the Tacktick wind
// transducer because of a poor signal/noise ratio, you can ask MicronetToNMEA to repeat
// the values of AWA & AWS on the network by setting MICRONET_WIND_REPEATER to 1. Set it
// to 0 else.
#define MICRONET_WIND_REPEATER 1

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* BOARDCONFIG_H_ */
