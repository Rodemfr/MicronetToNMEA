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

#include <Arduino.h>
#include <string.h>
#include <cmath>
#include "MicronetCodec.h"
#include "Version.h"
#include "Globals.h"
#include "NavigationData.h"

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

MicronetCodec::MicronetCodec()
{
}

MicronetCodec::~MicronetCodec()
{
}

uint32_t MicronetCodec::GetNetworkId(MicronetMessage_t *message)
{
	unsigned int networkId;

	networkId = message->data[MICRONET_NUID_OFFSET];
	networkId = (networkId << 8) | message->data[MICRONET_NUID_OFFSET + 1];
	networkId = (networkId << 8) | message->data[MICRONET_NUID_OFFSET + 2];
	networkId = (networkId << 8) | message->data[MICRONET_NUID_OFFSET + 3];

	return networkId;
}

uint8_t MicronetCodec::GetDeviceType(MicronetMessage_t *message)
{
	return message->data[MICRONET_DT_OFFSET];
}

uint32_t MicronetCodec::GetDeviceId(MicronetMessage_t *message)
{
	unsigned int deviceId;

	deviceId = message->data[MICRONET_DUID_OFFSET];
	deviceId = (deviceId << 8) | message->data[MICRONET_DUID_OFFSET + 1];
	deviceId = (deviceId << 8) | message->data[MICRONET_DUID_OFFSET + 2];
	deviceId = (deviceId << 8) | message->data[MICRONET_DUID_OFFSET + 3];

	return deviceId;
}

uint8_t MicronetCodec::GetMessageId(MicronetMessage_t *message)
{
	return message->data[MICRONET_MI_OFFSET];
}

uint8_t MicronetCodec::GetSource(MicronetMessage_t *message)
{
	return message->data[MICRONET_DF_OFFSET];
}

uint8_t MicronetCodec::GetSignalStrength(MicronetMessage_t *message)
{
	return message->data[MICRONET_SS_OFFSET];
}

uint8_t MicronetCodec::GetHeaderCrc(MicronetMessage_t *message)
{
	return message->data[MICRONET_CRC_OFFSET];
}

bool MicronetCodec::VerifyHeaderCrc(MicronetMessage_t *message)
{
	if (message->len < 14)
		return false;

	if (message->data[MICRONET_LEN_OFFSET_1] != message->data[MICRONET_LEN_OFFSET_2])
		return false;

	uint8_t crc = 0;
	for (int i = 0; i < MICRONET_CRC_OFFSET; i++)
	{
		crc += message->data[i];
	}

	return (crc == message->data[MICRONET_CRC_OFFSET]);
}

bool MicronetCodec::DecodeMessage(MicronetMessage_t *message, NavigationData *dataSet)
{
	bool ackRequested = false;

	switch (message->data[MICRONET_MI_OFFSET])
	{
	case MICRONET_MESSAGE_ID_SEND_DATA:
		DecodeSendDataMessage(message, dataSet);
		break;
	case MICRONET_MESSAGE_ID_SET_PARAMETER:
		DecodeSetParameterMessage(message, dataSet);
		ackRequested = true;
		break;
	}

	return ackRequested;
}

void MicronetCodec::DecodeSendDataMessage(MicronetMessage_t *message, NavigationData *dataSet)
{
	int fieldOffset = MICRONET_PAYLOAD_OFFSET;
	while (fieldOffset < message->len)
	{
		fieldOffset = DecodeDataField(message, fieldOffset, dataSet);
		if (fieldOffset < 0)
		{
			break;
		}
	}
	CalculateTrueWind(dataSet);
}

void MicronetCodec::DecodeSetParameterMessage(MicronetMessage_t *message, NavigationData *dataSet)
{
	switch (message->data[MICRONET_PAYLOAD_OFFSET + 1])
	{
	case MICRONET_CALIBRATION_WATER_SPEED_FACTOR_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int32_t value = (uint8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			value -= 0x32;
			dataSet->waterSpeedFactor_per = 1.0f + (((float) value) / 100.0f);
			dataSet->calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_WIND_SPEED_FACTOR_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			dataSet->windSpeedFactor_per = 1.0f + (((float) value) / 100.0f);
			dataSet->calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_WATER_TEMP_OFFSET_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			dataSet->waterTemperatureOffset_degc = ((float) value) / 2.0f;
			dataSet->calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_DEPTH_OFFSET_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			dataSet->depthOffset_m = ((float) value) * 0.3048f / 10.0f;
			dataSet->calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_WINDIR_OFFSET_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 2)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 4];
			value <<= 8;
			value |= (uint8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			dataSet->windDirectionOffset_deg = (float) value;
			dataSet->calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_HEADING_OFFSET_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 2)
		{
			int32_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 4];
			value <<= 8;
			value |= (uint8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			dataSet->headingOffset_deg = (float) value;
			dataSet->calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_MAGVAR_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			int8_t value = (int8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			dataSet->magneticVariation_deg = (float) value;
			dataSet->calibrationUpdated = true;
		}
		break;
	case MICRONET_CALIBRATION_WIND_SHIFT_ID:
		if (message->data[MICRONET_PAYLOAD_OFFSET + 2] == 1)
		{
			uint8_t value = (uint8_t) message->data[MICRONET_PAYLOAD_OFFSET + 3];
			dataSet->windShift_min = (float) value;
			dataSet->calibrationUpdated = true;
		}
		break;
	}
}

int MicronetCodec::DecodeDataField(MicronetMessage_t *message, int offset, NavigationData *dataSet)
{
	int8_t value8;
	int16_t value16;
	int32_t value_32_1, value32_2;

	if (message->data[offset] == MICRONET_FIELD_TYPE_3)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2] + message->data[offset + 3];
		if (crc == message->data[offset + 4])
		{
			value8 = message->data[offset + 3];
			UpdateMicronetData(message->data[offset + 1], value8, dataSet);
		}
	}
	else if (message->data[offset] == MICRONET_FIELD_TYPE_4)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2] + message->data[offset + 3]
				+ message->data[offset + 4];
		if (crc == message->data[offset + 5])
		{
			value16 = message->data[offset + 3];
			value16 = (value16 << 8) | message->data[offset + 4];
			UpdateMicronetData(message->data[offset + 1], value16, dataSet);
		}
	}
	else if (message->data[offset] == MICRONET_FIELD_TYPE_5)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2] + message->data[offset + 3]
				+ message->data[offset + 4] + message->data[offset + 5];
		if (crc == message->data[offset + 6])
		{
			value16 = message->data[offset + 3];
			value16 = (value16 << 8) | message->data[offset + 4];
			UpdateMicronetData(message->data[offset + 1], value16, dataSet);
		}
	}
	else if (message->data[offset] == MICRONET_FIELD_TYPE_A)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2] + message->data[offset + 3]
				+ message->data[offset + 4] + message->data[offset + 5] + message->data[offset + 6] + message->data[offset + 7]
				+ message->data[offset + 8] + message->data[offset + 9] + message->data[offset + 10];
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
			UpdateMicronetData(message->data[offset + 1], value_32_1, value32_2, dataSet);
		}
	}

	return offset + message->data[offset] + 2;
}

void MicronetCodec::UpdateMicronetData(uint8_t fieldId, int8_t value, NavigationData *dataSet)
{
	switch (fieldId)
	{
	case MICRONET_FIELD_ID_STP:
		dataSet->stp_degc.value = (((float) value) / 2.0f) + dataSet->waterTemperatureOffset_degc;
		dataSet->stp_degc.valid = true;
		dataSet->stp_degc.timeStamp = millis();
		break;
	}
}

void MicronetCodec::UpdateMicronetData(uint8_t fieldId, int16_t value, NavigationData *dataSet)
{
	float newValue;

	switch (fieldId)
	{
	case MICRONET_FIELD_ID_STW:
		dataSet->stw_kt.value = (((float) value) / 100.0f) * dataSet->waterSpeedFactor_per;
		dataSet->stw_kt.valid = true;
		dataSet->stw_kt.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_DPT:
		if (value < MAXIMUM_VALID_DEPTH_FT * 10)
		{
			dataSet->dpt_m.value = (((float) value) * 0.3048f / 10.0f) + dataSet->depthOffset_m;
			dataSet->dpt_m.valid = true;
			dataSet->dpt_m.timeStamp = millis();
		}
		else
		{
			dataSet->dpt_m.valid = false;
		}
		break;
	case MICRONET_FIELD_ID_AWS:
		dataSet->aws_kt.value = (((float) value) / 10.0f) * dataSet->windSpeedFactor_per;
		dataSet->aws_kt.valid = true;
		dataSet->aws_kt.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_AWA:
		newValue = ((float) value) + dataSet->windDirectionOffset_deg;
		if (newValue > 180.0f)
			newValue -= 360.0f;
		if (newValue < -180.0f)
			newValue += 360.0f;
		dataSet->awa_deg.value = newValue;
		dataSet->awa_deg.valid = true;
		dataSet->awa_deg.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_VCC:
		dataSet->vcc_v.value = ((float) value) / 10.0f;
		dataSet->vcc_v.valid = true;
		dataSet->vcc_v.timeStamp = millis();
		break;
	}
}

void MicronetCodec::UpdateMicronetData(uint8_t fieldId, int32_t value1, int32_t value2, NavigationData *dataSet)
{
	switch (fieldId)
	{
	case MICRONET_FIELD_ID_LOG:
		dataSet->trip_nm.value = ((float) value1) / 100.0f;
		dataSet->trip_nm.valid = true;
		dataSet->trip_nm.timeStamp = millis();
		dataSet->log_nm.value = ((float) value2) / 10.0f;
		dataSet->log_nm.valid = true;
		dataSet->log_nm.timeStamp = millis();
		break;
	}
}

void MicronetCodec::CalculateTrueWind(NavigationData *dataSet)
{
	if ((dataSet->awa_deg.valid) && (dataSet->aws_kt.valid) && (dataSet->stw_kt.valid))
	{
		if ((!dataSet->twa_deg.valid) || (!dataSet->tws_kt.valid) || (dataSet->awa_deg.timeStamp > dataSet->twa_deg.timeStamp)
				|| (dataSet->aws_kt.timeStamp > dataSet->tws_kt.timeStamp)
				|| (dataSet->stw_kt.timeStamp > dataSet->twa_deg.timeStamp))
		{
			float twLon, twLat;
			twLon = (dataSet->aws_kt.value * cosf(dataSet->awa_deg.value * M_PI / 180.0f)) - dataSet->stw_kt.value;
			twLat = (dataSet->aws_kt.value * sinf(dataSet->awa_deg.value * M_PI / 180.0f));

			dataSet->tws_kt.value = sqrtf(twLon * twLon + twLat * twLat);
			dataSet->tws_kt.valid = true;
			dataSet->tws_kt.timeStamp = millis();

			dataSet->twa_deg.value = atan2f(twLat, twLon) * 180.0f / M_PI;
			dataSet->twa_deg.valid = true;
			dataSet->twa_deg.timeStamp = millis();
		}
	}
}

uint8_t MicronetCodec::GetDataMessageLength(uint32_t dataFields)
{
	int offset = 0;

	if (dataFields & DATA_FIELD_TIME)
	{
		offset += 6;
	}
	if (dataFields & DATA_FIELD_DATE)
	{
		offset += 7;
	}
	if (dataFields & DATA_FIELD_SOGCOG)
	{
		offset += 8;
	}
	if (dataFields & DATA_FIELD_POSITION)
	{
		offset += 11;
	}
	if (dataFields & DATA_FIELD_XTE)
	{
		offset += 6;
	}
	if (dataFields & DATA_FIELD_DTW)
	{
		offset += 8;
	}
	if (dataFields & DATA_FIELD_BTW)
	{
		offset += 12;
	}
	if (dataFields & DATA_FIELD_VMGWP)
	{
		offset += 6;
	}
	if (dataFields & DATA_FIELD_HDG)
	{
		offset += 6;
	}

	return offset;
}

uint8_t MicronetCodec::EncodeDataMessage(MicronetMessage_t *message, uint8_t signalStrength, uint32_t networkId,
		uint32_t deviceId, NavigationData *navData, uint32_t dataFields)
{
	int offset = 0;

	// Network ID
	message->data[offset++] = (networkId >> 24) & 0xff;
	message->data[offset++] = (networkId >> 16) & 0xff;
	message->data[offset++] = (networkId >> 8) & 0xff;
	message->data[offset++] = networkId & 0xff;
	// Device ID
	message->data[offset++] = (deviceId >> 24) & 0xff;
	message->data[offset++] = (deviceId >> 16) & 0xff;
	message->data[offset++] = (deviceId >> 8) & 0xff;
	message->data[offset++] = deviceId & 0xff;
	// Message info
	message->data[offset++] = MICRONET_MESSAGE_ID_SEND_DATA;
	message->data[offset++] = 0x01;
	message->data[offset++] = signalStrength;
	// Header CRC
	message->data[offset++] = 0x00;
	// Message size
	message->data[offset++] = 0x00;
	message->data[offset++] = 0x00;
	// Data fields
	if ((dataFields & DATA_FIELD_TIME) && (navData->time.valid))
	{
		offset += Add16bitField(message->data + offset, MICRONET_FIELD_ID_TIME, (navData->time.hour << 8) + navData->time.minute);
	}
	if ((dataFields & DATA_FIELD_DATE) && (navData->date.valid))
	{
		offset += Add24bitField(message->data + offset, MICRONET_FIELD_ID_DATE,
				(navData->date.day << 16) + (navData->date.month << 8) + navData->date.year);
	}
	if ((dataFields & DATA_FIELD_SOGCOG) && ((navData->sog_kt.valid) || (navData->cog_deg.valid)))
	{
		offset += AddDual16bitField(message->data + offset, MICRONET_FIELD_ID_SOGCOG, navData->sog_kt.value * 10.0f,
				navData->cog_deg.value);
	}
	if ((dataFields & DATA_FIELD_POSITION) && ((navData->latitude_deg.valid) || (navData->longitude_deg.valid)))
	{
		offset += AddPositionField(message->data + offset, navData->latitude_deg.value, navData->longitude_deg.value);
	}
	if ((dataFields & DATA_FIELD_XTE) && (navData->xte_nm.valid))
	{
		offset += Add16bitField(message->data + offset, MICRONET_FIELD_ID_XTE, (short) (navData->xte_nm.value * 100));
	}
	if ((dataFields & DATA_FIELD_DTW) && (navData->dtw_nm.valid))
	{
		offset += Add32bitField(message->data + offset, MICRONET_FIELD_ID_DTW, (short) (navData->dtw_nm.value * 100));
	}
	if ((dataFields & DATA_FIELD_BTW) && (navData->btw_deg.valid))
	{
		offset += AddQuad16bitField(message->data + offset, MICRONET_FIELD_ID_BTW, (short) navData->btw_deg.value, 0, 0, 0);
	}
	if ((dataFields & DATA_FIELD_VMGWP) && (navData->vmc_kt.valid))
	{
		offset += Add16bitField(message->data + offset, MICRONET_FIELD_ID_VMGWP, (short) (navData->vmc_kt.value * 100));
	}
	if ((dataFields & DATA_FIELD_HDG) && (navData->hdg_deg.valid))
	{
		offset += Add16bitField(message->data + offset, MICRONET_FIELD_ID_HDG, navData->hdg_deg.value);
	}
	if ((dataFields & DATA_FIELD_NODE_INFO))
	{
		offset += AddQuad8bitField(message->data + offset, MICRONET_FIELD_ID_NODE_INFO, deviceId >> 24,
				MNET2NMEA_SW_MINOR_VERSION,
				MNET2NMEA_SW_MAJOR_VERSION, 0x33, signalStrength);
	}

	message->len = offset;

	WriteHeaderLengthAndCrc(message);

	return offset - MICRONET_PAYLOAD_OFFSET;
}

uint8_t MicronetCodec::EncodeSlotUpdateMessage(MicronetMessage_t *message, uint8_t signalStrength, uint32_t networkId,
		uint32_t deviceId, uint8_t payloadLength)
{
	int offset = 0;

	// Network ID
	message->data[offset++] = (networkId >> 24) & 0xff;
	message->data[offset++] = (networkId >> 16) & 0xff;
	message->data[offset++] = (networkId >> 8) & 0xff;
	message->data[offset++] = networkId & 0xff;
	// Device ID
	message->data[offset++] = (deviceId >> 24) & 0xff;
	message->data[offset++] = (deviceId >> 16) & 0xff;
	message->data[offset++] = (deviceId >> 8) & 0xff;
	message->data[offset++] = deviceId & 0xff;
	// Message info
	message->data[offset++] = MICRONET_MESSAGE_ID_UPDATE_SLOT;
	message->data[offset++] = 0x09;
	message->data[offset++] = signalStrength;
	// Header CRC
	message->data[offset++] = 0x00;
	// Message size
	message->data[offset++] = 0x00;
	message->data[offset++] = 0x00;
	// Data fields
	message->data[offset++] = payloadLength;

	uint8_t crc = 0;
	for (int i = MICRONET_PAYLOAD_OFFSET; i < offset; i++)
	{
		crc += message->data[i];
	}
	message->data[offset++] = crc;

	message->len = offset;

	WriteHeaderLengthAndCrc(message);

	return offset - MICRONET_PAYLOAD_OFFSET;
}

uint8_t MicronetCodec::EncodeSlotRequestMessage(MicronetMessage_t *message, uint8_t signalStrength, uint32_t networkId,
		uint32_t deviceId, uint8_t payloadLength)
{
	int offset = 0;

	// Network ID
	message->data[offset++] = (networkId >> 24) & 0xff;
	message->data[offset++] = (networkId >> 16) & 0xff;
	message->data[offset++] = (networkId >> 8) & 0xff;
	message->data[offset++] = networkId & 0xff;
	// Device ID
	message->data[offset++] = (deviceId >> 24) & 0xff;
	message->data[offset++] = (deviceId >> 16) & 0xff;
	message->data[offset++] = (deviceId >> 8) & 0xff;
	message->data[offset++] = deviceId & 0xff;
	// Message info
	message->data[offset++] = MICRONET_MESSAGE_ID_REQUEST_SLOT;
	message->data[offset++] = 0x09;
	message->data[offset++] = signalStrength;
	// Header CRC
	message->data[offset++] = 0x00;
	// Message size
	message->data[offset++] = 0x00;
	message->data[offset++] = 0x00;
	// Data fields
	message->data[offset++] = 0x00;
	message->data[offset++] = payloadLength;

	uint8_t crc = 0;
	for (int i = MICRONET_PAYLOAD_OFFSET; i < offset; i++)
	{
		crc += message->data[i];
	}
	message->data[offset++] = crc;

	message->len = offset;

	WriteHeaderLengthAndCrc(message);

	return offset - MICRONET_PAYLOAD_OFFSET;
}

uint8_t MicronetCodec::EncodeResetMessage(MicronetMessage_t *message, uint8_t signalStrength, uint32_t networkId,
		uint32_t deviceId)
{
	int offset = 0;

	// Network ID
	message->data[offset++] = (networkId >> 24) & 0xff;
	message->data[offset++] = (networkId >> 16) & 0xff;
	message->data[offset++] = (networkId >> 8) & 0xff;
	message->data[offset++] = networkId & 0xff;
	// Device ID
	message->data[offset++] = (deviceId >> 24) & 0xff;
	message->data[offset++] = (deviceId >> 16) & 0xff;
	message->data[offset++] = (deviceId >> 8) & 0xff;
	message->data[offset++] = deviceId & 0xff;
	// Message info
	message->data[offset++] = MICRONET_MESSAGE_ID_SET_PARAMETER;
	message->data[offset++] = 0x09;
	message->data[offset++] = signalStrength;
	// Header CRC
	message->data[offset++] = 0x00;
	// Message size
	message->data[offset++] = 0x00;
	message->data[offset++] = 0x00;
	// Data fields
	message->data[offset++] = 0xfa;
	message->data[offset++] = 0x4f;
	message->data[offset++] = 0x46;
	message->data[offset++] = 0x46;
	message->data[offset++] = 0x26;

	uint8_t crc = 0;
	for (int i = MICRONET_PAYLOAD_OFFSET; i < offset; i++)
	{
		crc += message->data[i];
	}
	message->data[offset++] = crc;

	message->len = offset;

	WriteHeaderLengthAndCrc(message);

	return offset - MICRONET_PAYLOAD_OFFSET;
}

uint8_t MicronetCodec::EncodeAckParamMessage(MicronetMessage_t *message, uint8_t signalStrength, uint32_t networkId,
		uint32_t deviceId)
{
	int offset = 0;

	// Network ID
	message->data[offset++] = (networkId >> 24) & 0xff;
	message->data[offset++] = (networkId >> 16) & 0xff;
	message->data[offset++] = (networkId >> 8) & 0xff;
	message->data[offset++] = networkId & 0xff;
	// Device ID
	message->data[offset++] = (deviceId >> 24) & 0xff;
	message->data[offset++] = (deviceId >> 16) & 0xff;
	message->data[offset++] = (deviceId >> 8) & 0xff;
	message->data[offset++] = deviceId & 0xff;
	// Message info
	message->data[offset++] = MICRONET_MESSAGE_ID_ACK_PARAMETER;
	message->data[offset++] = 0x01;
	message->data[offset++] = signalStrength;
	// Header CRC
	message->data[offset++] = 0x00;
	// Message size
	message->data[offset++] = 0x00;
	message->data[offset++] = 0x00;

	message->len = offset;

	WriteHeaderLengthAndCrc(message);

	return offset - MICRONET_PAYLOAD_OFFSET;
}

uint8_t MicronetCodec::EncodePingMessage(MicronetMessage_t *message, uint8_t signalStrength, uint32_t networkId,
		uint32_t deviceId)
{
	int offset = 0;

	// Network ID
	message->data[offset++] = (networkId >> 24) & 0xff;
	message->data[offset++] = (networkId >> 16) & 0xff;
	message->data[offset++] = (networkId >> 8) & 0xff;
	message->data[offset++] = networkId & 0xff;
	// Device ID
	message->data[offset++] = (deviceId >> 24) & 0xff;
	message->data[offset++] = (deviceId >> 16) & 0xff;
	message->data[offset++] = (deviceId >> 8) & 0xff;
	message->data[offset++] = deviceId & 0xff;
	// Message info
	message->data[offset++] = MICRONET_MESSAGE_ID_PING;
	message->data[offset++] = 0x09;
	message->data[offset++] = signalStrength;
	// Header CRC
	message->data[offset++] = 0x00;
	// Message size
	message->data[offset++] = 0x00;
	message->data[offset++] = 0x00;

	uint8_t crc = 0;
	for (int i = MICRONET_PAYLOAD_OFFSET; i < offset; i++)
	{
		crc += message->data[i];
	}
	message->data[offset++] = crc;

	message->len = offset;

	WriteHeaderLengthAndCrc(message);

	return offset - MICRONET_PAYLOAD_OFFSET;
}

void MicronetCodec::WriteHeaderLengthAndCrc(MicronetMessage_t *message)
{
	message->data[MICRONET_LEN_OFFSET_1] = message->len - 2;
	message->data[MICRONET_LEN_OFFSET_2] = message->len - 2;

	uint8_t crc = 0;
	for (int i = 0; i < MICRONET_CRC_OFFSET; i++)
	{
		crc += message->data[i];
	}

	message->data[MICRONET_CRC_OFFSET] = crc;
}

uint8_t MicronetCodec::Add16bitField(uint8_t *buffer, uint8_t fieldCode, int16_t value)
{
	int offset = 0;

	buffer[offset++] = 0x04;
	buffer[offset++] = fieldCode;
	buffer[offset++] = 0x05;

	buffer[offset++] = (value >> 8) & 0xff;
	buffer[offset++] = value & 0xff;

	uint8_t crc = 0;
	for (int i = offset - 5; i < offset; i++)
	{
		crc += buffer[i];
	}
	buffer[offset++] = crc;

	return offset;
}

uint8_t MicronetCodec::Add24bitField(uint8_t *buffer, uint8_t fieldCode, int32_t value)
{
	int offset = 0;

	buffer[offset++] = 0x05;
	buffer[offset++] = fieldCode;
	buffer[offset++] = 0x05;

	buffer[offset++] = (value >> 16) & 0xff;
	buffer[offset++] = (value >> 8) & 0xff;
	buffer[offset++] = value & 0xff;

	uint8_t crc = 0;
	for (int i = offset - 6; i < offset; i++)
	{
		crc += buffer[i];
	}
	buffer[offset++] = crc;

	return offset;
}

uint8_t MicronetCodec::AddDual16bitField(uint8_t *buffer, uint8_t fieldCode, int16_t value1, int16_t value2)
{
	int offset = 0;

	buffer[offset++] = 0x06;
	buffer[offset++] = fieldCode;
	buffer[offset++] = 0x05;

	buffer[offset++] = (value1 >> 8) & 0xff;
	buffer[offset++] = value1 & 0xff;
	buffer[offset++] = (value2 >> 8) & 0xff;
	buffer[offset++] = value2 & 0xff;

	uint8_t crc = 0;
	for (int i = offset - 7; i < offset; i++)
	{
		crc += buffer[i];
	}
	buffer[offset++] = crc;

	return offset;
}

uint8_t MicronetCodec::AddQuad8bitField(uint8_t *buffer, uint8_t fieldCode, uint8_t fieldData, uint8_t value1, uint8_t value2,
		uint8_t value3, uint8_t value4)
{
	int offset = 0;

	buffer[offset++] = 0x06;
	buffer[offset++] = fieldCode;
	buffer[offset++] = 0x03;

	buffer[offset++] = value1;
	buffer[offset++] = value2;
	buffer[offset++] = value3;
	buffer[offset++] = value4;

	uint8_t crc = 0;
	for (int i = offset - 7; i < offset; i++)
	{
		crc += buffer[i];
	}
	buffer[offset++] = crc;

	return offset;
}

uint8_t MicronetCodec::AddQuad16bitField(uint8_t *buffer, uint8_t fieldCode, int16_t value1, int16_t value2, int16_t value3,
		int16_t value4)
{
	int offset = 0;

	buffer[offset++] = 0x0a;
	buffer[offset++] = fieldCode;
	buffer[offset++] = 0x05;

	buffer[offset++] = (value1 >> 8) & 0xff;
	buffer[offset++] = value1 & 0xff;
	buffer[offset++] = (value2 >> 8) & 0xff;
	buffer[offset++] = value2 & 0xff;
	buffer[offset++] = (value3 >> 8) & 0xff;
	buffer[offset++] = value3 & 0xff;
	buffer[offset++] = (value4 >> 8) & 0xff;
	buffer[offset++] = value4 & 0xff;

	uint8_t crc = 0;
	for (int i = offset - 11; i < offset; i++)
	{
		crc += buffer[i];
	}
	buffer[offset++] = crc;

	return offset;
}

uint8_t MicronetCodec::Add32bitField(uint8_t *buffer, uint8_t fieldCode, int32_t value)
{
	int offset = 0;

	buffer[offset++] = 0x06;
	buffer[offset++] = fieldCode;
	buffer[offset++] = 0x05;

	buffer[offset++] = (value >> 24) & 0xff;
	buffer[offset++] = (value >> 16) & 0xff;
	buffer[offset++] = (value >> 8) & 0xff;
	buffer[offset++] = value & 0xff;

	uint8_t crc = 0;
	for (int i = offset - 7; i < offset; i++)
	{
		crc += buffer[i];
	}
	buffer[offset++] = crc;

	return offset;
}

uint8_t MicronetCodec::AddDual32bitField(uint8_t *buffer, uint8_t fieldCode, int32_t value1, int32_t value2)
{
	int offset = 0;

	buffer[offset++] = 0x0a;
	buffer[offset++] = fieldCode;
	buffer[offset++] = 0x05;

	buffer[offset++] = (value1 >> 24) & 0xff;
	buffer[offset++] = (value1 >> 16) & 0xff;
	buffer[offset++] = (value1 >> 8) & 0xff;
	buffer[offset++] = value1 & 0xff;
	buffer[offset++] = (value2 >> 24) & 0xff;
	buffer[offset++] = (value2 >> 16) & 0xff;
	buffer[offset++] = (value2 >> 8) & 0xff;
	buffer[offset++] = value2 & 0xff;

	uint8_t crc = 0;
	for (int i = offset - 11; i < offset; i++)
	{
		crc += buffer[i];
	}
	buffer[offset++] = crc;

	return offset;
}

uint8_t MicronetCodec::AddPositionField(uint8_t *buffer, float latitude, float longitude)
{
	int offset = 0;
	uint8_t dir = 0x0;

	buffer[offset++] = 0x09;
	buffer[offset++] = 0x09;
	buffer[offset++] = 0x05;

	// Direction flags
	if (latitude > 0.0f)
		dir |= 0x01;
	else
		latitude = -latitude;

	if (longitude > 0.0f)
		dir |= 0x02;
	else
		longitude = -longitude;

	// Latitude
	buffer[offset++] = (uint8_t) floorf(latitude);
	uint16_t latMin = 60000.0f * (latitude - floorf(latitude));
	buffer[offset++] = (latMin >> 8) & 0xff;
	buffer[offset++] = latMin & 0xff;

	// Longitude
	buffer[offset++] = (uint8_t) floorf(longitude);
	uint16_t lonMin = 60000.0f * (longitude - floorf(longitude));
	buffer[offset++] = (lonMin >> 8) & 0xff;
	buffer[offset++] = lonMin & 0xff;

	buffer[offset++] = dir;
	uint8_t crc = 0;
	for (int i = offset - 10; i < offset; i++)
	{
		crc += buffer[i];
	}
	buffer[offset++] = crc;

	return offset;
}

bool MicronetCodec::GetNetworkMap(MicronetMessage_t *message, NetworkMap *networkMap)
{
	uint32_t messageLength = message->len;
	uint32_t offset;
	uint32_t networkId;
	uint32_t nbDevices;
	uint32_t slotDelay_us;
	uint32_t slotLength_us;
	uint8_t payloadBytes;
	uint32_t deviceId;
	uint32_t slotIndex;

	if (message->data[MICRONET_MI_OFFSET] != MICRONET_MESSAGE_ID_MASTER_REQUEST)
	{
		return false;
	}

	uint8_t crc = 0;
	for (offset = MICRONET_PAYLOAD_OFFSET; offset < (uint32_t) (messageLength - 1); offset++)
	{
		crc += message->data[offset];
	}

	if (crc != message->data[messageLength - 1])
	{
		return false;
	}

	networkId = message->data[0] << 24;
	networkId |= message->data[1] << 16;
	networkId |= message->data[2] << 8;
	networkId |= message->data[3];
	networkMap->networkId = networkId;

	nbDevices = ((message->len - MICRONET_PAYLOAD_OFFSET - 3) / 5);
	networkMap->nbSyncSlots = 0;

	deviceId = message->data[MICRONET_PAYLOAD_OFFSET] << 24;
	deviceId |= message->data[MICRONET_PAYLOAD_OFFSET + 1] << 16;
	deviceId |= message->data[MICRONET_PAYLOAD_OFFSET + 2] << 8;
	deviceId |= message->data[MICRONET_PAYLOAD_OFFSET + 3];
	networkMap->masterDevice = deviceId;
	networkMap->networkStart = message->startTime_us;

	networkMap->firstSlot = message->endTime_us;

	slotDelay_us = 0;
	slotIndex = 0;
	for (uint32_t i = 1; i < nbDevices; i++)
	{
		deviceId = message->data[MICRONET_PAYLOAD_OFFSET + i * 5] << 24;
		deviceId |= message->data[MICRONET_PAYLOAD_OFFSET + i * 5 + 1] << 16;
		deviceId |= message->data[MICRONET_PAYLOAD_OFFSET + i * 5 + 2] << 8;
		deviceId |= message->data[MICRONET_PAYLOAD_OFFSET + i * 5 + 3];
		payloadBytes = message->data[MICRONET_PAYLOAD_OFFSET + i * 5 + 4];

		networkMap->syncSlot[slotIndex].deviceId = deviceId;
		networkMap->syncSlot[slotIndex].payloadBytes = payloadBytes;

		// A payload length of zero indicates that there is no slot reserved for the corresponding device.
		if (payloadBytes != 0)
		{
			networkMap->syncSlot[slotIndex].start_us = message->endTime_us + slotDelay_us;
			slotLength_us = PREAMBLE_LENGTH_IN_US + HEADER_LENGTH_IN_US + (payloadBytes * BYTE_LENGTH_IN_US) + GUARD_TIME_IN_US;
			slotLength_us = ((slotLength_us + WINDOW_ROUNDING_TIME_US - 1) / WINDOW_ROUNDING_TIME_US) * WINDOW_ROUNDING_TIME_US;
			slotDelay_us += slotLength_us;
			networkMap->syncSlot[slotIndex].length_us = slotLength_us;
		}
		else
		{
			networkMap->syncSlot[slotIndex].start_us = 0;
			networkMap->syncSlot[slotIndex].length_us = 0;
		}

		slotIndex++;
	}
	networkMap->nbSyncSlots = slotIndex;

	networkMap->asyncSlot.deviceId = 0;
	slotDelay_us += ASYNC_WINDOW_OFFSET;
	networkMap->asyncSlot.start_us = message->endTime_us + slotDelay_us;
	networkMap->asyncSlot.length_us = ASYNC_WINDOW_LENGTH;
	networkMap->asyncSlot.payloadBytes = ASYNC_WINDOW_PAYLOAD;
	slotDelay_us += ASYNC_WINDOW_LENGTH;

	for (uint32_t i = 0; i < networkMap->nbSyncSlots; i++)
	{
		networkMap->ackSlot[i].deviceId = networkMap->syncSlot[networkMap->nbSyncSlots - 1 - i].deviceId;
		networkMap->ackSlot[i].start_us = message->endTime_us + slotDelay_us;
		networkMap->ackSlot[i].length_us = ACK_WINDOW_LENGTH;
		networkMap->ackSlot[i].payloadBytes = ACK_WINDOW_PAYLOAD;
		slotDelay_us += ACK_WINDOW_LENGTH;
	}

	networkMap->ackSlot[networkMap->nbSyncSlots].deviceId = networkMap->masterDevice;
	networkMap->ackSlot[networkMap->nbSyncSlots].start_us = message->endTime_us + slotDelay_us;
	networkMap->ackSlot[networkMap->nbSyncSlots].length_us = ACK_WINDOW_LENGTH;
	networkMap->ackSlot[networkMap->nbSyncSlots].payloadBytes = ACK_WINDOW_PAYLOAD;
	networkMap->nbAckSlots = networkMap->nbSyncSlots + 1;
	slotDelay_us += ACK_WINDOW_LENGTH;

	networkMap->networkEnd = message->endTime_us + slotDelay_us;

	return true;
}

TxSlotDesc_t MicronetCodec::GetSyncTransmissionSlot(NetworkMap *networkMap, uint32_t deviceId)
{
	for (uint32_t i = 0; i < networkMap->nbSyncSlots; i++)
	{
		if (networkMap->syncSlot[i].deviceId == deviceId)
		{
			return networkMap->syncSlot[i];
		}
	}

	return
	{	0, 0, 0, 0};
}

TxSlotDesc_t MicronetCodec::GetAsyncTransmissionSlot(NetworkMap *networkMap)
{
	return networkMap->asyncSlot;
}

TxSlotDesc_t MicronetCodec::GetAckTransmissionSlot(NetworkMap *networkMap, uint32_t deviceId)
{
	for (uint32_t i = 0; i < networkMap->nbAckSlots; i++)
	{
		if (networkMap->ackSlot[i].deviceId == deviceId)
			return networkMap->ackSlot[i];
	}

	return
	{	0, 0, 0, 0};
}

uint32_t MicronetCodec::GetStartOfNetwork(NetworkMap *networkMap)
{
	return networkMap->networkStart;
}

uint32_t MicronetCodec::GetNextStartOfNetwork(NetworkMap *networkMap)
{
	return networkMap->networkStart + 1000000;
}

uint32_t MicronetCodec::GetEndOfNetwork(NetworkMap *networkMap)
{
	return networkMap->networkEnd;
}

uint8_t MicronetCodec::CalculateSignalStrength(MicronetMessage_t *message)
{
	int16_t rssi = message->rssi;

	if (rssi < -95)
		return 0;
	else if (rssi < -90)
		return 1;
	else if (rssi < -85)
		return 2;
	else if (rssi < -80)
		return 3;
	else if (rssi < -75)
		return 4;
	else if (rssi < -70)
		return 5;
	else if (rssi < -65)
		return 6;
	else if (rssi < -60)
		return 7;
	else if (rssi < -55)
		return 8;
	else
		return 9;
}

float MicronetCodec::CalculateSignalFloatStrength(MicronetMessage_t *message)
{
	float strength = (message->rssi + 95) / 5.0;

	if (strength < 0)
		strength = 0;

	return strength;
}
