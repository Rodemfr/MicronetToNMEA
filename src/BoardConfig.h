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

// Select 868MHz or 915MHz Micronet frequency
// 0 -> 868Mhz
// 1 -> 915Mhz
#define FREQUENCY_SYSTEM 0

// Selects on which I2C bus is connected compass as per Wiring library definition
#define COMPASS_I2C Wire

// CC1101/SPI pins
#define CS0_PIN  10
#define MOSI_PIN 11
#define MISO_PIN 12
#define SCK_PIN  13
#define GDO0_PIN 9

// Console UART params
#define CONSOLE          SerialUSB
#define CONSOLE_BAUDRATE 115200

// UBlox GNSS UART pins
#define GNSS          Serial3
#define GNSS_BAUDRATE 9600
#define GNSS_RX_PIN   15
#define GNSS_TX_PIN   14

// AIS UART pins
#define AIS          Serial2
#define AIS_BAUDRATE 38400
#define AIS_RX_PIN   7

// Plotter/Nav Computer UART params
#define PLOTTER          Serial1
#define PLOTTER_BAUDRATE 115200
#define PLOTTER_RX_PIN   0
#define PLOTTER_TX_PIN   1

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* BOARDCONFIG_H_ */
