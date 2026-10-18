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

#pragma once

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

// Select 868MHz or 915MHz Micronet frequency
// 0 -> 868Mhz
// 1 -> 915Mhz
#define FREQUENCY_SYSTEM 0

// CC1101/SPI pins
#define CS0_PIN  5
#define MOSI_PIN 23
#define MISO_PIN 19
#define SCK_PIN  18
#define GDO0_PIN GPIO_NUM_35

// I2C bus used by the compass modules
#define COMPASS_I2C_PORT    I2C_NUM_0
#define COMPASS_I2C_SDA_PIN GPIO_NUM_21
#define COMPASS_I2C_SCL_PIN GPIO_NUM_22

// LED GPIO
#define LED_GPIO GPIO_NUM_2

// Console is on Bluetooth SPP link
#define CONSOLE gBtMultiSPP
// Plotter is on Bluetooth SPP link
#define PLOTTER gBtMultiSPP

// NMEA receiver is on Serial1 link
#define NMEA0183_IN          gSerial1
#define NMEA0183_IN_BAUDRATE 38400
#define NMEA0183_IN_RX_PIN   GPIO_NUM_27
#define NMEA0183_IN_TX_PIN   GPIO_NUM_26

// NMEA0183 input configuration
#define GNSS             gSerial2
#define GNSS_BAUDRATE    9600
#define GNSS_RX_PIN      GPIO_NUM_16
#define GNSS_TX_PIN      GPIO_NUM_17
#define GNSS_IS_UBLOXM8N 1 // Set to 1 to enable UBlox M6N/M8N automatic configuration

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/
