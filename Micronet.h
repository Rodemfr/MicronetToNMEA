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

#ifndef MICRONET_H_
#define MICRONET_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include <stdint.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define MICRONET_MAX_MESSAGE_LENGTH 80
#define MICRONET_MIN_MESSAGE_LENGTH 13

#define MICRONET_NUID_OFFSET    0
#define MICRONET_DUID_OFFSET    4
#define MICRONET_DT_OFFSET      4
#define MICRONET_MI_OFFSET      8
#define MICRONET_SO_OFFSET      9
#define MICRONET_DE_OFFSET      10
#define MICRONET_CRC_OFFSET     11
#define MICRONET_LEN_OFFSET_1   12
#define MICRONET_LEN_OFFSET_2   13
#define MICRONET_PAYLOAD_OFFSET 14

#define MICRONET_MESSAGE_ID_REQUEST_DATA    0x01
#define MICRONET_MESSAGE_ID_SEND_DATA       0x02
#define MICRONET_MESSAGE_ID_SET_CALIBRATION 0x06

#define MICRONET_FIELD_TYPE_3 0x03
#define MICRONET_FIELD_TYPE_4 0x04
#define MICRONET_FIELD_TYPE_5 0x05
#define MICRONET_FIELD_TYPE_A 0x0a

#define MICRONET_FIELD_ID_STW 0x01
#define MICRONET_FIELD_ID_LOG 0x02
#define MICRONET_FIELD_ID_STP 0x03
#define MICRONET_FIELD_ID_DPT 0x04
#define MICRONET_FIELD_ID_AWS 0x05
#define MICRONET_FIELD_ID_AWA 0x06
#define MICRONET_FIELD_ID_VCC 0x1b

#define MICRONET_CALIBRATION_WATER_SPEED_FACTOR_ID 0x04
#define MICRONET_CALIBRATION_SPEED_FILTERING_ID    0x04
#define MICRONET_CALIBRATION_SPEED_FACTOR_ID       0x06
#define MICRONET_CALIBRATION_WINDIR_OFFSET_ID      0x07

#define MICRONET_SPEED_FILTERING_AUTO   0x00
#define MICRONET_SPEED_FILTERING_SLOW   0x10
#define MICRONET_SPEED_FILTERING_MEDIUM 0x20
#define MICRONET_SPEED_FILTERING_FAST   0x30

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

typedef struct
{
	uint8_t len;
	int16_t rssi;
	uint8_t data[MICRONET_MAX_MESSAGE_LENGTH];
} MicronetMessage_t;

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* MICRONET_H_ */
