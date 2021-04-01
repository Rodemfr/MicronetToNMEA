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

#include "MicronetCodec.h"
#include "Globals.h"
#include <Arduino.h>
#include <string.h>
#include <cmath>
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
	return message->data[MICRONET_SO_OFFSET];
}

uint8_t MicronetCodec::GetDestination(MicronetMessage_t *message)
{
	return message->data[MICRONET_DE_OFFSET];
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

void MicronetCodec::DecodeMessage(MicronetMessage_t *message, NavigationData *dataSet)
{
	switch (message->data[MICRONET_MI_OFFSET])
	{
	case MICRONET_MESSAGE_ID_SEND_DATA:
		DecodeSendDataMessage(message, dataSet);
		break;
	case MICRONET_MESSAGE_ID_SET_CALIBRATION:
		DecodeSetParameterMessage(message, dataSet);
		break;
	}

	// TODO : make invalid values which have not been updated for too long (3s ?)
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
			dataSet->waterTemperatureOffset_C = ((float) value) / 2.0f;
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
			dataSet->windShift = (float) value;
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
		dataSet->stp.value = (((float) value) / 2.0f) + dataSet->waterTemperatureOffset_C;
		dataSet->stp.valid = true;
		dataSet->stp.timeStamp = millis();
		break;
	}
}

void MicronetCodec::UpdateMicronetData(uint8_t fieldId, int16_t value, NavigationData *dataSet)
{
	float newValue;

	switch (fieldId)
	{
	case MICRONET_FIELD_ID_STW:
		dataSet->stw.value = (((float) value) / 100.0f) * dataSet->waterSpeedFactor_per;
		dataSet->stw.valid = true;
		dataSet->stw.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_DPT:
		if (value < MAXIMUM_VALID_DEPTH_FT * 10)
		{
			dataSet->dpt.value = (((float) value) * 0.3048f / 10.0f) + dataSet->depthOffset_m;
			dataSet->dpt.valid = true;
			dataSet->dpt.timeStamp = millis();
		}
		else
		{
			dataSet->dpt.valid = false;
		}
		break;
	case MICRONET_FIELD_ID_AWS:
		dataSet->aws.value = (((float) value) / 10.0f) * dataSet->windSpeedFactor_per;
		dataSet->aws.valid = true;
		dataSet->aws.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_AWA:
		newValue = ((float) value) + dataSet->windDirectionOffset_deg;
		if (newValue > 180.0f)
			newValue -= 360.0f;
		if (newValue < -180.0f)
			newValue += 360.0f;
		dataSet->awa.value = newValue;
		dataSet->awa.valid = true;
		dataSet->awa.timeStamp = millis();
		break;
	case MICRONET_FIELD_ID_VCC:
		dataSet->vcc.value = ((float) value) / 10.0f;
		dataSet->vcc.valid = true;
		dataSet->vcc.timeStamp = millis();
		break;
	}
}

void MicronetCodec::UpdateMicronetData(uint8_t fieldId, int32_t value1, int32_t value2, NavigationData *dataSet)
{
	switch (fieldId)
	{
	case MICRONET_FIELD_ID_LOG:
		// TODO : Shall speed factor be applied on log ?
		dataSet->trip.value = ((float) value1) / 100.0f;
		dataSet->trip.valid = true;
		dataSet->trip.timeStamp = millis();
		dataSet->log.value = ((float) value2) / 10.0f;
		dataSet->log.valid = true;
		dataSet->log.timeStamp = millis();
		break;
	}
}

void MicronetCodec::CalculateTrueWind(NavigationData *dataSet)
{
	if ((dataSet->awa.valid) && (dataSet->aws.valid) && (dataSet->stw.valid))
	{
		if ((!dataSet->twa.valid) || (!dataSet->tws.valid) || (dataSet->awa.timeStamp > dataSet->twa.timeStamp)
				|| (dataSet->aws.timeStamp > dataSet->tws.timeStamp)
				|| (dataSet->stw.timeStamp > dataSet->twa.timeStamp))
		{
			float twLon, twLat;
			twLon = (dataSet->aws.value * cosf(dataSet->awa.value * M_PI / 180.0f)) - dataSet->stw.value;
			twLat = (dataSet->aws.value * sinf(dataSet->awa.value * M_PI / 180.0f));

			dataSet->tws.value = sqrtf(twLon * twLon + twLat * twLat);
			dataSet->tws.valid = true;
			dataSet->tws.timeStamp = millis();

			dataSet->twa.value = atan2f(twLat, twLon) * 180.0f / M_PI;
			dataSet->twa.valid = true;
			dataSet->twa.timeStamp = millis();
		}
	}
}

bool MicronetCodec::BuildGnssMessage(MicronetMessage_t *message, uint32_t networkId, NmeaData_t *nmeaData)
{
	int offset = 0;

	if ((!nmeaData->timeUpdated) && (!nmeaData->dateUpdated) && (!nmeaData->sogCogUpdated) && (!nmeaData->positionUpdated))
	{
		message->len = 0;
		return false;
	}

	// Network ID
	message->data[offset++] = (networkId >> 24) & 0xff;
	message->data[offset++] = (networkId >> 16) & 0xff;
	message->data[offset++] = (networkId >> 8) & 0xff;
	message->data[offset++] = networkId & 0xff;
	// Device ID
	message->data[offset++] = (gConfiguration.deviceId >> 24) & 0xff;
	message->data[offset++] = (gConfiguration.deviceId >> 16) & 0xff;
	message->data[offset++] = (gConfiguration.deviceId >> 8) & 0xff;
	message->data[offset++] = gConfiguration.deviceId & 0xff;
	// Message info
	message->data[offset++] = 0x02;
	message->data[offset++] = 0x01;
	message->data[offset++] = 0x09;
	// Header CRC
	message->data[offset++] = 0x00;
	// Message size
	message->data[offset++] = 0x0c;
	message->data[offset++] = 0x0c;
	// Data fields
	if (nmeaData->timeUpdated)
	{
		nmeaData->timeUpdated = false;
		offset += Add16bitField(message->data + offset, MICRONET_FIELD_ID_TIME, (nmeaData->hour << 8) + nmeaData->minute);
	}
	if (nmeaData->dateUpdated)
	{
		nmeaData->dateUpdated = false;
		offset += Add24bitField(message->data + offset, MICRONET_FIELD_ID_DATE,
				(nmeaData->day << 16) + (nmeaData->month << 8) + nmeaData->year);
	}
	if (nmeaData->sogCogUpdated)
	{
		nmeaData->sogCogUpdated = false;
		offset += AddDual16bitField(message->data + offset, MICRONET_FIELD_ID_SOGCOG, nmeaData->sog, nmeaData->cog);
	}
	if (nmeaData->positionUpdated)
	{
		nmeaData->positionUpdated = false;
		offset += AddPositionField(message->data + offset, nmeaData->latitude, nmeaData->longitude);
	}

	message->len = offset;

	WriteHeaderLengthAndCrc(message);

	return true;
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
	if (longitude > 0.0f)
		dir |= 0x02;
	// Latitude
	latitude = fabsf(latitude);
	buffer[offset++] = (uint8_t) floorf(latitude);
	uint16_t latMin = 60000.0f * (latitude - floorf(latitude));
	buffer[offset++] = (latMin >> 8) & 0xff;
	buffer[offset++] = latMin & 0xff;
	// Longitude
	longitude = fabsf(longitude);
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

uint32_t MicronetCodec::GetTransmissionSlot(MicronetMessage_t *message)
{
	uint32_t messageLength = message->len;
	uint32_t offset;
	uint32_t txDelayUs;
	uint32_t nbDevices, nbSlots;
	uint32_t payloadBits;

	uint8_t crc = 0;
	for (offset = MICRONET_PAYLOAD_OFFSET; offset < (uint32_t) (messageLength - 1); offset++)
	{
		crc += message->data[offset];
	}

	if (crc != message->data[messageLength - 1])
		return 0;

	nbDevices = ((message->len - MICRONET_PAYLOAD_OFFSET - 3) / 5);

	payloadBits = 0;
	nbSlots = 0;
	for (uint32_t i = 1; i < nbDevices; i++)
	{
		// A payload length of zero indicates that there is not slot reserved for the corresponding device.
		if (message->data[MICRONET_PAYLOAD_OFFSET + i * 5 + 4] != 0)
		{
			nbSlots++;
			payloadBits += message->data[MICRONET_PAYLOAD_OFFSET + i * 5 + 4] << 3;
		}
	}

	txDelayUs = ((GUARD_TIME_IN_BITS + nbSlots * (GUARD_TIME_IN_BITS + PREAMBLE_LENGTH_IN_BITS) + payloadBits) * BIT_LENGTH_IN_NS) / 1000;
	txDelayUs += 10000;

	return message->timeStamp_us + txDelayUs;
}
