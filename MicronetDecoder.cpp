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
		DecodeSetParameterMessage(message);
		break;
	}

	// TODO : make invalid values which have not been updated for too long (3s ?)
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

void MicronetDecoder::DecodeSetParameterMessage(MicronetMessage_t *message)
{
	switch (message->data[MICRONET_PAYLOAD_OFFSET + 1])
	{
	case MICRONET_CALIBRATION_WATER_SPEED_FACTOR_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int32_t value = (uint8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			value -= 0x32;
			micronetData.waterSpeedFactor_per = 1.0f + (((float) value) / 100.0f);
			micronetData.calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_WIND_SPEED_FACTOR_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			micronetData.windSpeedFactor_per = 1.0f + (((float) value) / 100.0f);
			micronetData.calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_WATER_TEMP_OFFSET_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			micronetData.waterTemperatureOffset_C = ((float) value) / 2.0f;
			micronetData.calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_DEPTH_OFFSET_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			micronetData.depthOffset_m = ((float) value) * 0.3048f / 10.0f;
			micronetData.calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_WINDIR_OFFSET_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 2)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 4];
			value <<= 8;
			value |= (uint8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			micronetData.windDirectionOffset_deg = (float) value;
			micronetData.calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_HEADING_OFFSET_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 2)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 4];
			value <<= 8;
			value |= (uint8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			micronetData.headingOffset_deg = (float) value;
			micronetData.calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_MAGVAR_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int8_t value = (int8_t)message->data[MICRONET_PAYLOAD_OFFSET + 3];
			micronetData.magneticVariation_deg = (float) value;
			micronetData.calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_WIND_SHIFT_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			uint8_t value = (uint8_t)message->data[MICRONET_PAYLOAD_OFFSET + 3];
			micronetData.windShift = (float) value;
			micronetData.calibrationUpdated = true;
		}
		break;
	}
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
		micronetData.stp.value = (((float) value) / 2.0f) + micronetData.waterTemperatureOffset_C;
		micronetData.stp.valid = true;
		micronetData.stp.timeStamp = millis();
		break;
	}
}

void MicronetDecoder::UpdateMicronetData(uint8_t fieldId, int16_t value)
{
	float newValue;

	switch (fieldId)
	{
	case MICRONET_FIELD_ID_STW:
		micronetData.stw.value = (((float) value) / 100.0f) * micronetData.waterSpeedFactor_per;
		micronetData.stw.valid = true;
		micronetData.stw.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_DPT:
		if (value < MAXIMUM_VALID_DEPTH_FT * 10)
		{
			micronetData.dpt.value = (((float) value) * 0.3048f / 10.0f) + micronetData.depthOffset_m;
			micronetData.dpt.valid = true;
			micronetData.dpt.timeStamp = millis();
		}
		else
		{
			micronetData.dpt.valid = false;
		}
		break;
	case MICRONET_FIELD_ID_AWS:
		micronetData.aws.value = (((float) value) / 10.0f) * micronetData.windSpeedFactor_per;
		micronetData.aws.valid = true;
		micronetData.aws.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_AWA:
		newValue = ((float) value) + micronetData.windDirectionOffset_deg;
		if (newValue > 180.0f) newValue -= 360.0f;
		if (newValue < -180.0f) newValue += 360.0f;
		micronetData.awa.value = newValue;
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
		// TODO : Shall speed factor be applied on log ?
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
