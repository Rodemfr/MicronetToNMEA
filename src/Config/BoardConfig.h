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
#define GDO0_PIN 35

// Selects on which I2C bus is connected compass as per Wiring library definition
#define COMPASS_I2C Wire

// Bluetooth device name as it will be seen by other devices
#define BLUETOOTH_DEVICE_NAME "MicronetToNMEA"

// Console UART params. Use 'Serial' for USB console, or 'gBtSerial' for Bluetooth console
#define CONSOLE          Serial
#define CONSOLE_BAUDRATE 115200

// NMEA0183 input configuration
#define NMEA0183_IN_IS_UBLOXM8N 1 // Set to 1 to enable UBlox M6N/M8N automatic configuration
#define NMEA0183_IN             Serial2
#define NMEA0183_IN_BAUDRATE    9600

// Plotter/Nav Computer UART params. Use 'Serial' for USB link, or 'gBtSerial' for Bluetooth link
#define PLOTTER Serial

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/
