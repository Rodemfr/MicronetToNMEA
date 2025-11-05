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

// Console UART params
#define CONSOLE          Serial
#define CONSOLE_BAUDRATE 115200

// UBlox GNSS UART pins
#define GNSS          Serial1
#define GNSS_BAUDRATE 9600
#define GNSS_RX_PIN   16
#define GNSS_TX_PIN   17

// AIS UART pins
#define AIS          Serial2
#define AIS_BAUDRATE 38400
#define AIS_RX_PIN   34

// Plotter/Nav Computer UART params
#define PLOTTER gBTSerial

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

