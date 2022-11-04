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

#include "BoardConfig.h"
#include <stdint.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#if (FREQUENCY_SYSTEM == 1)
#define MICRONET_RF_CENTER_FREQUENCY_MHZ 915.914
#else
#define MICRONET_RF_CENTER_FREQUENCY_MHZ 869.840
#endif
// Note that the actual deviation on Micronet devices is around 38kHz and not 34
// However, CC1101 produces 38 kHz deviation when requested 34 : problem of HW, Driver or measurement tools ?
#define MICRONET_RF_DEVIATION_KHZ    34
#define MICRONET_RF_BAUDRATE_BAUD    76800

#define MICRONET_RF_PREAMBLE_LENGTH  15
#define MICRONET_RF_PREAMBLE_BYTE    0x55
#define MICRONET_RF_SYNC_BYTE        0x99

#define MICRONET_MAX_MESSAGE_LENGTH 96
#define MICRONET_MIN_MESSAGE_LENGTH 13

#define MICRONET_NUID_OFFSET    0
#define MICRONET_DUID_OFFSET    4
#define MICRONET_DT_OFFSET      4
#define MICRONET_MI_OFFSET      8
#define MICRONET_DF_OFFSET      9
#define MICRONET_SS_OFFSET      10
#define MICRONET_CRC_OFFSET     11
#define MICRONET_LEN_OFFSET_1   12
#define MICRONET_LEN_OFFSET_2   13
#define MICRONET_PAYLOAD_OFFSET 14

#define MICRONET_MESSAGE_ID_MASTER_REQUEST 0x01
#define MICRONET_MESSAGE_ID_SEND_DATA      0x02
#define MICRONET_MESSAGE_ID_REQUEST_SLOT   0x03
#define MICRONET_MESSAGE_ID_UPDATE_SLOT    0x05
#define MICRONET_MESSAGE_ID_SET_PARAMETER  0x06
#define MICRONET_MESSAGE_ID_ACK_PARAMETER  0x07
#define MICRONET_MESSAGE_ID_PING           0x0a
#define MICRONET_MESSAGE_ID_ACK_PING       0x0b

#define MICRONET_DEVICE_TYPE_HULL_TRANSMITTER    0x01
#define MICRONET_DEVICE_TYPE_WIND_TRANSDUCER     0x02
#define MICRONET_DEVICE_TYPE_NMEA_CONVERTER      0x03
#define MICRONET_DEVICE_TYPE_MAST_ROTATION       0x04
#define MICRONET_DEVICE_TYPE_MOB                 0x05
#define MICRONET_DEVICE_TYPE_SDPOD               0x06
#define MICRONET_DEVICE_TYPE_DUAL_DISPLAY        0x81
#define MICRONET_DEVICE_TYPE_ANALOG_WIND_DISPLAY 0x83

#define MICRONET_FIELD_TYPE_3 0x03
#define MICRONET_FIELD_TYPE_4 0x04
#define MICRONET_FIELD_TYPE_5 0x05
#define MICRONET_FIELD_TYPE_A 0x0a

#define MICRONET_FIELD_ID_SPD      0x01
#define MICRONET_FIELD_ID_LOG      0x02
#define MICRONET_FIELD_ID_STP      0x03
#define MICRONET_FIELD_ID_DPT      0x04
#define MICRONET_FIELD_ID_AWS      0x05
#define MICRONET_FIELD_ID_AWA      0x06
#define MICRONET_FIELD_ID_HDG      0x07
#define MICRONET_FIELD_ID_SOGCOG   0x08
#define MICRONET_FIELD_ID_LATLON   0x09
#define MICRONET_FIELD_ID_BTW      0x0a
#define MICRONET_FIELD_ID_XTE      0x0b
#define MICRONET_FIELD_ID_TIME     0x0c
#define MICRONET_FIELD_ID_DATE     0x0d
#define MICRONET_FIELD_ID_NODE_INFO 0x10
#define MICRONET_FIELD_ID_VMGWP    0x12
#define MICRONET_FIELD_ID_VCC      0x1b
#define MICRONET_FIELD_ID_DTW      0x1f
#define MICRONET_FIELD_ID_RAWS     0x21
#define MICRONET_FIELD_ID_RAWA     0x22

#define MICRONET_CALIBRATION_WATER_SPEED_FACTOR_ID 0x00
#define MICRONET_CALIBRATION_WATER_TEMP_OFFSET_ID  0x02
#define MICRONET_CALIBRATION_DEPTH_OFFSET_ID       0x03
#define MICRONET_CALIBRATION_SPEED_FILTERING_ID    0x04
#define MICRONET_CALIBRATION_WIND_SPEED_FACTOR_ID  0x06
#define MICRONET_CALIBRATION_WINDIR_OFFSET_ID      0x07
#define MICRONET_CALIBRATION_HEADING_OFFSET_ID     0x09
#define MICRONET_CALIBRATION_MAGVAR_ID             0x0d
#define MICRONET_CALIBRATION_WIND_SHIFT_ID         0x0e

#define BYTE_LENGTH_IN_US       104
#define WINDOW_ROUNDING_TIME_US 244
#define PREAMBLE_LENGTH_IN_US   1771
#define HEADER_LENGTH_IN_US     1456
#define GUARD_TIME_IN_US        1893
#define ASYNC_WINDOW_OFFSET     7200
#define ASYNC_WINDOW_LENGTH		7200
#define ASYNC_WINDOW_PAYLOAD	20
#define ACK_WINDOW_LENGTH		5328
#define ACK_WINDOW_PAYLOAD      2

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

#define MICRONET_ACTION_RF_NO_ACTION    0
#define MICRONET_ACTION_RF_LOW_POWER    1
#define MICRONET_ACTION_RF_ACTIVE_POWER 2

typedef struct
{
	uint8_t action;
	uint8_t len;
	int16_t rssi;
	uint32_t startTime_us;
	uint32_t endTime_us;
	uint8_t data[MICRONET_MAX_MESSAGE_LENGTH];
} MicronetMessage_t;

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* MICRONET_H_ */
