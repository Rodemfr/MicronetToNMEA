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
#define NAVCOMPASS_I2C Wire1 // SDA0: 18 SCL0: 19
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

// ERROR LED pin
#define LED_PIN LED_BUILTIN

// NMEA GNSS UART pins
#if defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
#define GNSS_UBLOXM8N  1       // Set to one if your GNSS is a UBLOX M8N, 0 else. If set to one, GNSS will be automatically configured at startup
#define GNSS_SERIAL    Serial1
#define GNSS_BAUDRATE  9600
#define GNSS_RX_PIN    0
#define GNSS_TX_PIN    1
#else // Teensy 4.0 Configuration
#define GNSS_UBLOXM8N  0       // Set to one if your GNSS is a UBLOX M8N, 0 else. If set to one, GNSS will be automatically configured at startup
#define GNSS_SERIAL    Serial5
#define GNSS_BAUDRATE  9600
#define GNSS_RX_PIN    0
#define GNSS_TX_PIN    1
#endif

// USB UART params
#define USB_NMEA     Serial
#define USB_BAUDRATE 115200

// Wired UART params
#if defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
#define WIRED_NMEA     Serial5
#define WIRED_BAUDRATE 115200
#define WIRED_RX_PIN   34
#define WIRED_TX_PIN   33
#else // Teensy 4.0 Configuration
#define WIRED_NMEA     Serial5
#define WIRED_BAUDRATE 115200
#define WIRED_RX_PIN   21
#define WIRED_TX_PIN   20
#endif
// The console to use for menu and NMEA output
#define CONSOLE  USB_NMEA
#define NMEA_OUT USB_NMEA
#define NMEA_IN  USB_NMEA

// Defines with data comes from which link
// LINK_NMEA_EXT -> data comes from external NMEA link
// LINK_NMEA_GNSS -> data comes from GNSS NMEA link
// LINK_MICRONET -> data comes from Micronet network
// LINK_COMPASS -> data comes from LSM303
#define NAV_SOURCE_LINK     LINK_NMEA_EXT // Navigation data (RMB)
#define GNSS_SOURCE_LINK    LINK_NMEA_EXT // Positionning data (RMC, GGA, VTG)
#define WIND_SOURCE_LINK    LINK_NMEA_EXT // Wind data (MWV)
#define DEPTH_SOURCE_LINK   LINK_NMEA_EXT // Depth data (DPT)
#define SPEED_SOURCE_LINK   LINK_NMEA_EXT // Speed data (SPD, LOG)
#define VOLTAGE_SOURCE_LINK LINK_NMEA_EXT // Battery voltage data (XDG)
#define SEATEMP_SOURCE_LINK LINK_NMEA_EXT // Temperature data (STP)
#define COMPASS_SOURCE_LINK LINK_NMEA_EXT // Heading data (HDG)

//#define NAV_SOURCE_LINK     LINK_NMEA_EXT
//#define GNSS_SOURCE_LINK    LINK_NMEA_GNSS
//#define WIND_SOURCE_LINK    LINK_MICRONET
//#define DEPTH_SOURCE_LINK   LINK_MICRONET
//#define SPEED_SOURCE_LINK   LINK_MICRONET
//#define VOLTAGE_SOURCE_LINK LINK_MICRONET
//#define SEATEMP_SOURCE_LINK LINK_MICRONET
//#define COMPASS_SOURCE_LINK LINK_COMPASS

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* BOARDCONFIG_H_ */
