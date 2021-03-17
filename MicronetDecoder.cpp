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

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "MicronetDecoder.h"

#include <Arduino.h>
#include <string.h>
#include <cmath>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define MAXIMUM_VALID_DEPTH_FT 500

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

MicronetDecoder::MicronetDecoder()
{
	memset(&micronetData, 0, sizeof(micronetData));
}

MicronetDecoder::~MicronetDecoder()
{
}

uint32_t MicronetDecoder::GetNetworkId(MicronetMessage_t *message)
{
	unsigned int networkId;

	networkId = message->data[MICRONET_NUID_OFFSET];
	networkId = (networkId << 8) | message->data[MICRONET_NUID_OFFSET + 1];
	networkId = (networkId << 8) | message->data[MICRONET_NUID_OFFSET + 2];
	networkId = (networkId << 8) | message->data[MICRONET_NUID_OFFSET + 3];

	return networkId;
}

uint8_t MicronetDecoder::GetDeviceType(MicronetMessage_t *message)
{
	return message->data[MICRONET_DT_OFFSET];
}

uint32_t MicronetDecoder::GetDeviceId(MicronetMessage_t *message)
{
	unsigned int deviceId;

	deviceId = message->data[MICRONET_DUID_OFFSET];
	deviceId = (deviceId << 8) | message->data[MICRONET_DUID_OFFSET + 1];
	deviceId = (deviceId << 8) | message->data[MICRONET_DUID_OFFSET + 2];
	deviceId = (deviceId << 8) | message->data[MICRONET_DUID_OFFSET + 3];

	return deviceId;
}

uint8_t MicronetDecoder::GetMessageId(MicronetMessage_t *message)
{
	return message->data[MICRONET_MI_OFFSET];
}

uint8_t MicronetDecoder::GetSource(MicronetMessage_t *message)
{
	return message->data[MICRONET_SO_OFFSET];
}

uint8_t MicronetDecoder::GetDestination(MicronetMessage_t *message)
{
	return message->data[MICRONET_DE_OFFSET];
}

uint8_t MicronetDecoder::GetHeaderCrc(MicronetMessage_t *message)
{
	return message->data[MICRONET_CRC_OFFSET];
}

bool MicronetDecoder::VerifyHeaderCrc(MicronetMessage_t *message)
{
	uint8_t crc = 0;
	for (int i = 0; i < MICRONET_CRC_OFFSET; i++)
	{
		crc += message->data[i];
	}

	return (crc == message->data[MICRONET_CRC_OFFSET]);
}

void MicronetDecoder::DecodeMessage(MicronetMessage_t *message)
{
	switch (message->data[MICRONET_MI_OFFSET])
	{
	case MICRONET_MESSAGE_ID_SEND_DATA:
		DecodeSendDataMessage(message);
		break;
	case MICRONET_MESSAGE_ID_SET_CALIBRATION:
		DecodeSetCalibrationMessage(message);
		break;
	}
}

void MicronetDecoder::DecodeSendDataMessage(MicronetMessage_t *message)
{
	int fieldOffset = MICRONET_PAYLOAD_OFFSET;
	while (fieldOffset < message->len)
	{
		fieldOffset = DecodeDataField(message, fieldOffset);
		if (fieldOffset < 0)
		{
			break;
		}
	}
	CalculateTrueWind();
}

void MicronetDecoder::DecodeSetCalibrationMessage(MicronetMessage_t *message)
{
	switch (message->data[MICRONET_PAYLOAD_OFFSET + 1])
	{
	case MICRONET_CALIBRATION_WATER_SPEED_FACTOR_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int8_t value = message->data[MICRONET_PAYLOAD_OFFSET + 3];
		}
		break;
	case MICRONET_CALIBRATION_SPEED_FACTOR_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int8_t value = message->data[MICRONET_PAYLOAD_OFFSET + 3];
		}
		break;
	}

//	   0x00 Water speed factor
//	      VA = 8bit unsigned integer. Value is 0x32 + speed correction in % (e.g. 0x30<=>-2%, 0x37<=>+5%)
//	   0x02 Water temperature offset
//	      VA = 8bit signed interger. Value is temperature offset * 2, coded in Celsius
//	   0x03 Distance from depth transducer to waterline or keel
//	      VA = 8bit signed interger of the offset in ft*10. If the value is positive, it is the distance to waterline. If negative, to the keel.
//	   0x04 Speed filtering level
//	      VA = 0x00 : AUTO
//	      VA = 0x10 : SLOW
//	      VA = 0x20 : MED
//	      VA = 0x30 : FAST
//	   0x05 Wind Speed or Compass heading filtering level
//	      VA = 0x00 : AUTO (Wind Speed)
//	      VA = 0x01 : SLOW (Wind Speed)
//	      VA = 0x02 : MED (Wind Speed)
//	      VA = 0x03 : FAST (Wind Speed)
//	      VA = 0x00 : AUTO (Heading) - I see the same code for both Wind and Compass here ? How does the device distinguish ?
//	      VA = 0x10 : SLOW (Heading)
//	      VA = 0x20 : MED (Heading)
//	      VA = 0x30 : FAST (Heading)
//	   0x06 Wind speed factor
//	      VA = Speed correction as signed 8bit interger in percent
//	   0x07 Wind direction offset
//	      VA = An signed 16-bit integer in degrees /!\ LITTLE ENDIAN VALUE /!\
//	   0x09 Compass heading direction offset
//	      VA = An signed 16-bit integer in degrees /!\ LITTLE ENDIAN VALUE /!\
//	   0x0D Compass magnetic variation
//	      VA = Variation as signed 8bit interger in degrees
//	   0x0E Wind shift
//	      VA = 8bit unsigned integer

}

int MicronetDecoder::DecodeDataField(MicronetMessage_t *message, int offset)
{
	int8_t value8;
	int16_t value16;
	int32_t value_32_1, value32_2;

	if (message->data[offset] == MICRONET_FIELD_TYPE_3)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2]
				+ message->data[offset + 3];
		if (crc == message->data[offset + 4])
		{
			value8 = message->data[offset + 3];
			UpdateMicronetData(message->data[offset + 1], value8);
		}
	}
	else if (message->data[offset] == MICRONET_FIELD_TYPE_4)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2]
				+ message->data[offset + 3] + message->data[offset + 4];
		if (crc == message->data[offset + 5])
		{
			value16 = message->data[offset + 3];
			value16 = (value16 << 8) | message->data[offset + 4];
			UpdateMicronetData(message->data[offset + 1], value16);
		}
	}
	else if (message->data[offset] == MICRONET_FIELD_TYPE_5)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2]
				+ message->data[offset + 3] + message->data[offset + 4] + message->data[offset + 5];
		if (crc == message->data[offset + 6])
		{
			value16 = message->data[offset + 3];
			value16 = (value16 << 8) | message->data[offset + 4];
			UpdateMicronetData(message->data[offset + 1], value16);
		}
	}
	else if (message->data[offset] == MICRONET_FIELD_TYPE_A)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2]
				+ message->data[offset + 3] + message->data[offset + 4] + message->data[offset + 5]
				+ message->data[offset + 6] + message->data[offset + 7] + message->data[offset + 8]
				+ message->data[offset + 9] + message->data[offset + 10];
		if (crc == message->data[offset + 11])
		{
			value_32_1 = message->data[offset + 3];
			value_32_1 = (value_32_1 << 8) | message->data[offset + 4];
			value_32_1 = (value_32_1 << 8) | message->data[offset + 5];
			value_32_1 = (value_32_1 << 8) | message->data[offset + 6];
			value32_2 = message->data[offset + 7];
			value32_2 = (value32_2 << 8) | message->data[offset + 8];
			value32_2 = (value32_2 << 8) | message->data[offset + 9];
			value32_2 = (value32_2 << 8) | message->data[offset + 10];
			UpdateMicronetData(message->data[offset + 1], value_32_1, value32_2);
		}
	}

	return offset + message->data[offset] + 2;
}

void MicronetDecoder::UpdateMicronetData(uint8_t fieldId, int8_t value)
{
	switch (fieldId)
	{
	case MICRONET_FIELD_ID_STP:
		micronetData.stp.value = ((float) value) / 2.0f;
		micronetData.stp.valid = true;
		micronetData.stp.timeStamp = millis();
		break;
	}
}

void MicronetDecoder::UpdateMicronetData(uint8_t fieldId, int16_t value)
{
	switch (fieldId)
	{
	case MICRONET_FIELD_ID_STW:
		micronetData.stw.value = ((float) value) / 100.0f;
		micronetData.stw.valid = true;
		micronetData.stw.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_DPT:
		if (value < MAXIMUM_VALID_DEPTH_FT * 10)
		{
			micronetData.dpt.value = ((float) value) * 0.3048f / 10.0f;
			micronetData.dpt.valid = true;
			micronetData.dpt.timeStamp = millis();
		}
		else
		{
			micronetData.dpt.valid = false;
		}
		break;
	case MICRONET_FIELD_ID_AWS:
		micronetData.aws.value = ((float) value) / 10.0f;
		micronetData.aws.valid = true;
		micronetData.aws.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_AWA:
		micronetData.awa.value = (float) value;
		micronetData.awa.valid = true;
		micronetData.awa.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_VCC:
		micronetData.vcc.value = ((float) value) / 10.0f;
		micronetData.vcc.valid = true;
		micronetData.vcc.timeStamp = millis();
		break;
	}
}

void MicronetDecoder::UpdateMicronetData(uint8_t fieldId, int32_t value1, int32_t value2)
{
	switch (fieldId)
	{
	case MICRONET_FIELD_ID_LOG:
		micronetData.trip.value = ((float) value1) / 100.0f;
		micronetData.trip.valid = true;
		micronetData.trip.timeStamp = millis();
		micronetData.log.value = ((float) value2) / 10.0f;
		micronetData.log.valid = true;
		micronetData.log.timeStamp = millis();
		break;
	}
}

MicronetData_t* MicronetDecoder::GetCurrentData()
{
	return &micronetData;
}

void MicronetDecoder::CalculateTrueWind()
{
	if ((micronetData.awa.valid) && (micronetData.aws.valid) && (micronetData.stw.valid))
	{
		if ((!micronetData.twa.valid) || (!micronetData.tws.valid)
				|| (micronetData.awa.timeStamp > micronetData.twa.timeStamp)
				|| (micronetData.aws.timeStamp > micronetData.tws.timeStamp)
				|| (micronetData.stw.timeStamp > micronetData.twa.timeStamp))
		{
			float twLon, twLat;
			twLon = (micronetData.aws.value * cosf(micronetData.awa.value * M_PI / 180.0f)) - micronetData.stw.value;
			twLat = (micronetData.aws.value * sinf(micronetData.awa.value * M_PI / 180.0f));

			micronetData.tws.value = sqrtf(twLon * twLon + twLat * twLat);
			micronetData.tws.valid = true;
			micronetData.tws.timeStamp = millis();

			micronetData.twa.value = atan2f(twLat, twLon) * 180.0f / M_PI;
			micronetData.twa.valid = true;
			micronetData.twa.timeStamp = millis();
		}
	}
}
