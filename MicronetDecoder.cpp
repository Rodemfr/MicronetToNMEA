/*
 * PacketDecoder.cpp
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#include "MicronetDecoder.h"

#include <arduino.h>
#include <string.h>

#define MICRONET_MESSAGE_ID_REQUEST_DATA 0x01
#define MICRONET_MESSAGE_ID_SEND_DATA    0x02

#define MICRONET_FIELD_TYPE_4 0x04
#define MICRONET_FIELD_TYPE_5 0x05

#define MICRONET_FIELD_ID_DPT 0x04
#define MICRONET_FIELD_ID_AWS 0x05
#define MICRONET_FIELD_ID_AWA 0x06
#define MICRONET_FIELD_ID_VCC 0x1b
#define MICRONET_FIELD_ID_TWS 0x21
#define MICRONET_FIELD_ID_TWA 0x22

MicronetDecoder::MicronetDecoder()
{
	memset(&micronetData, 0, sizeof(micronetData));
	memset(&dataTimeStamps, 0, sizeof(dataTimeStamps));
}

MicronetDecoder::~MicronetDecoder()
{
}

uint32_t MicronetDecoder::GetNetworkId(MicronetMessage_t *message)
{
	unsigned int networkId;

	networkId = message->data[0];
	networkId = (networkId << 8) | message->data[1];
	networkId = (networkId << 8) | message->data[2];

	return networkId;
}

uint8_t MicronetDecoder::GetDeviceType(MicronetMessage_t *message)
{
	return message->data[3];
}

uint32_t MicronetDecoder::GetDeviceId(MicronetMessage_t *message)
{
	unsigned int deviceId;

	deviceId = message->data[4];
	deviceId = (deviceId << 8) | message->data[5];
	deviceId = (deviceId << 8) | message->data[6];

	return deviceId;
}

uint8_t MicronetDecoder::GetMessageCategory(MicronetMessage_t *message)
{
	return message->data[7];
}

uint8_t MicronetDecoder::GetMessageId(MicronetMessage_t *message)
{
	return message->data[8];
}

void MicronetDecoder::DecodeMessage(MicronetMessage_t *message)
{
	if (message->data[7] == MICRONET_MESSAGE_ID_SEND_DATA)
	{
		int fieldOffset = 13;
		while (fieldOffset < message->len)
		{
			fieldOffset = DecodeDataField(message, fieldOffset);
			if (fieldOffset < 0)
			{
				break;
			}
		}
	}
}

int MicronetDecoder::DecodeDataField(MicronetMessage_t *message, int offset)
{
	short value;

	if (message->data[offset] == MICRONET_FIELD_TYPE_4)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2] + message->data[offset + 3]
				+ message->data[offset + 4];
		if (crc == message->data[offset + 5])
		{
			value = message->data[offset + 3];
			value = (value << 8) | message->data[offset + 4];
			UpdateMicronetData(message->data[offset + 1], value);
		}

		return offset + 6;
	}
	else if (message->data[offset] == MICRONET_FIELD_TYPE_5)
	{
		uint8_t crc = message->data[offset] + message->data[offset + 1] + message->data[offset + 2] + message->data[offset + 3]
				+ message->data[offset + 4] + message->data[offset + 5];
		if (crc == message->data[offset + 6])
		{
			value = message->data[offset + 3];
			value = (value << 8) | message->data[offset + 4];
			UpdateMicronetData(message->data[offset + 1], value);
		}
		return offset + 7;
	}
	else
	{
		return -1;
	}
}

void MicronetDecoder::UpdateMicronetData(uint8_t fieldId, int16_t value)
{
	switch (fieldId)
	{
	case MICRONET_FIELD_ID_DPT:
		micronetData.depthM = ((float) value) / 10.0f;
		micronetData.depthValid = true;
		break;
	case MICRONET_FIELD_ID_AWS:
		micronetData.awsKt = ((float) value) / 10.0f;
		micronetData.awsValid = true;
		break;
	case MICRONET_FIELD_ID_AWA:
		micronetData.awaDeg = (float) value;
		micronetData.awaValid = true;
		break;
	case MICRONET_FIELD_ID_VCC:
		micronetData.vccV = ((float) value) / 10.0f;
		micronetData.vccValid = true;
		break;
	case MICRONET_FIELD_ID_TWS:
		micronetData.twsKt = ((float) value) / 10.0f;
		micronetData.twsValid = true;
		break;
	case MICRONET_FIELD_ID_TWA:
		micronetData.twaDeg = (float) value;
		micronetData.twaValid = true;
		break;
	}
}

void MicronetDecoder::PrintRawMessage(MicronetMessage_t *message)
{
	for (int j = 0; j < message->len; j++)
	{
		if (message->data[j] < 16)
		{
			Serial.print("0");
		}
		Serial.print(message->data[j], HEX);
		Serial.print(" ");
	}
	Serial.print(" (");
	Serial.print((int) message->len);
	Serial.print(",");
	Serial.print((int) message->rssi);
	Serial.print(")");

	Serial.println();
}

void MicronetDecoder::PrintCurrentData()
{
	if (micronetData.awaValid)
	{
		Serial.print("AWA ");
		Serial.print(micronetData.awaDeg);
		Serial.print(" | ");
	}
	if (micronetData.awsValid)
	{
		Serial.print("AWS ");
		Serial.print(micronetData.awsKt);
		Serial.print(" | ");
	}
	if (micronetData.twaValid)
	{
		Serial.print("TWA ");
		Serial.print(micronetData.twaDeg);
		Serial.print(" | ");
	}
	if (micronetData.twsValid)
	{
		Serial.print("TWS ");
		Serial.print(micronetData.twsKt);
		Serial.print(" | ");
	}
	if (micronetData.stwValid)
	{
		Serial.print("STW ");
		Serial.print(micronetData.stwKt);
		Serial.print(" | ");
	}
	if (micronetData.depthValid)
	{
		Serial.print("DPT ");
		Serial.print(micronetData.depthM);
		Serial.print(" | ");
	}
	if (micronetData.vccValid)
	{
		Serial.print("VCC ");
		Serial.print(micronetData.vccV);
		Serial.print(" | ");
	}

	Serial.println();
}

MicronetData_t* MicronetDecoder::GetCurrentData()
{
	return &micronetData;
}
