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
#define COMPASS_I2C    Wire
#define LSM303AGR_CSM  3
#define LSM303AGR_CSXL 4

// CC1101/SPI pins
#define CS0_PIN  10
#define MOSI_PIN 11
#define MISO_PIN 12
#define SCK_PIN  13
#define GDO0_PIN 9

// UBlox GNSS UART pins
#define GNSS          Serial3
#define GNSS_BAUDRATE 9600
#define GNSS_RX_PIN   15
#define GNSS_TX_PIN   14

// Console UART params
#define CONSOLE          SerialUSB
#define CONSOLE_BAUDRATE 115200

// Nav Computer/Plotter UART params
#define NAV_NMEA          Serial1
#define NAV_NMEA_BAUDRATE 115200
#define NAV_NMEA_RX_PIN   0
#define NAV_NMEA_TX_PIN   1

// Defines which data comes from which link
// LINK_NAV      -> External navigation computer
// LINK_GNSS     -> Internal UBlox GNSS link
// LINK_MICRONET -> Micronet network
// LINK_COMPASS  -> Internal LSM303
#define NAV_SOURCE_LINK     LINK_NAV      // Navigation data (RMB)
#define GNSS_SOURCE_LINK    LINK_GNSS     // Positionning data (RMC, GGA, VTG)
#define WIND_SOURCE_LINK    LINK_MICRONET // Wind data (MWV)
#define DEPTH_SOURCE_LINK   LINK_MICRONET // Depth data (DPT)
#define SPEED_SOURCE_LINK   LINK_MICRONET // Speed data (VHW, VLW)
#define VOLTAGE_SOURCE_LINK LINK_MICRONET // Battery voltage data (XDR)
#define SEATEMP_SOURCE_LINK LINK_MICRONET // Temperature data (MTW)
#define COMPASS_SOURCE_LINK LINK_COMPASS  // Heading data (HDG)

// Enable COG/SOG filtering.
// This functionnality reduces COG/SOG noise from GNSS at the cost of responsiveness.
// 0 -> disabled
// 1 -> enabled
#define SOG_COG_FILTERING 1
// Depth of COG/SOG filter [1..20]
#define SOG_COG_FILTERING_DEPTH 7

// Emulate water speed (SPD) with SOG from GNSS
// To be used when you don't have a speedo in your network
// 0 -> disabled
// 1 -> enabled
#define EMULATE_SPD_WITH_SOG 0

// Define which compass axis will be compared to magnetic north.
// This is the axis which will point to the bow of the boat.
// Set one of the (X, Y, Z) to 1.0 or -1.0
#define HEADING_AXIS                                                                                                                                 \
    {                                                                                                                                                \
        0.0f, 0.0f, -1.0f                                                                                                                            \
    }

// Define which compass axis will be used to compute roll angle.
// This is the vertical axis which will point down to the bottom of the boat.
// Set one of the (X, Y, Z) to 1.0 or -1.0
#define DOWN_AXIS                                                                                                                                    \
    {                                                                                                                                                \
        -1.0f, 0.0f, 0.0f                                                                                                                            \
    }

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* BOARDCONFIG_H_ */
