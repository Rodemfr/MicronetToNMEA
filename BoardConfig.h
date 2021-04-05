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

// CC1101/SPI pins
#define CS0_PIN              10
#define MOSI_PIN             11
#define MISO_PIN             12
#define SCK_PIN              14
#define GDO0_PIN             24
#define GDO2_PIN             25

// ERROR LED pin
#define LED_PIN              LED_BUILTIN

// Default packet length for RF reception. Must be less than 64.
#define RF_DEFAULT_PACKET_LENGTH 60

// NMEA/GNSS UART pins
#define GNSS_SERIAL   Serial1
#define GNSS_BAUDRATE 38400
#define GNSS_CALLBACK serialEvent1
#define GNSS_RX_PIN   0
#define GNSS_TX_PIN   1

// Wired console params
#define USB_CONSOLE  Serial
#define USB_BAUDRATE 38400

// Wireless Bluetooth console params
#define BLU_CONSOLE  Serial4
#define BLU_BAUDRATE 38400
#define BLU_RX_PIN   31
#define BLU_TX_PIN   32

// The console to use for menu and NMEA output
#define CONSOLE BLU_CONSOLE
#define NMEA_OUT USB_CONSOLE
#define NMEA_IN USB_CONSOLE

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* BOARDCONFIG_H_ */
