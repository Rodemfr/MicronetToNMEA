/*
 * PacketDecoder.cpp
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#include "PacketDecoder.h"

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

PacketDecoder::PacketDecoder()
{
	memset(&micronetData, 0, sizeof(micronetData));
	memset(&dataTimeStamps, 0, sizeof(dataTimeStamps));
}

PacketDecoder::~PacketDecoder()
{
}

uint32_t PacketDecoder::GetNetworkId(MicronetPacket_t *packet)
{
	unsigned int networkId;

	networkId = packet->data[0];
	networkId = (networkId << 8) | packet->data[1];
	networkId = (networkId << 8) | packet->data[2];

	return networkId;
}

uint8_t PacketDecoder::GetDeviceType(MicronetPacket_t *packet)
{
	return packet->data[3];
}

uint32_t PacketDecoder::GetDeviceId(MicronetPacket_t *packet)
{
	unsigned int deviceId;

	deviceId = packet->data[4];
	deviceId = (deviceId << 8) | packet->data[5];
	deviceId = (deviceId << 8) | packet->data[6];

	return deviceId;
}

uint8_t PacketDecoder::GetMessageCategory(MicronetPacket_t *packet)
{
	return packet->data[7];
}

uint8_t PacketDecoder::GetMessageId(MicronetPacket_t *packet)
{
	return packet->data[8];
}

void PacketDecoder::DecodeMessage(MicronetPacket_t *packet)
{
	if (packet->data[7] == MICRONET_MESSAGE_ID_SEND_DATA)
	{
		int fieldOffset = 13;
		while (fieldOffset < packet->len)
		{
			fieldOffset = DecodeDataField(packet, fieldOffset);
			if (fieldOffset < 0)
			{
				break;
			}
		}
	}
}

int PacketDecoder::DecodeDataField(MicronetPacket_t *packet, int offset)
{
	short value;

	if (packet->data[offset] == MICRONET_FIELD_TYPE_4)
	{
		uint8_t crc = packet->data[offset] + packet->data[offset + 1] + packet->data[offset + 2] + packet->data[offset + 3]
				+ packet->data[offset + 4];
		if (crc == packet->data[offset + 5])
		{
			value = packet->data[offset + 3];
			value = (value << 8) | packet->data[offset + 4];
			UpdateMicronetData(packet->data[offset + 1], value);
		}

		return offset + 6;
	}
	else if (packet->data[offset] == MICRONET_FIELD_TYPE_5)
	{
		uint8_t crc = packet->data[offset] + packet->data[offset + 1] + packet->data[offset + 2] + packet->data[offset + 3]
				+ packet->data[offset + 4] + packet->data[offset + 5];
		if (crc == packet->data[offset + 6])
		{
			value = packet->data[offset + 3];
			value = (value << 8) | packet->data[offset + 4];
			UpdateMicronetData(packet->data[offset + 1], value);
		}
		return offset + 7;
	}
	else
	{
		return -1;
	}
}

void PacketDecoder::UpdateMicronetData(uint8_t fieldId, int16_t value)
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

void PacketDecoder::PrintRawMessage(MicronetPacket_t *packet)
{
	for (int j = 0; j < packet->len; j++)
	{
		if (packet->data[j] < 16)
		{
			Serial.print("0");
		}
		Serial.print(packet->data[j], HEX);
		Serial.print(" ");
	}
	Serial.print(" (");
	Serial.print((int) packet->len);
	Serial.print(",");
	Serial.print((int) packet->rssi);
	Serial.print(")");

	Serial.println();
}

void PacketDecoder::PrintCurrentData()
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

MicronetData_t* PacketDecoder::GetCurrentData()
{
	return &micronetData;
}
