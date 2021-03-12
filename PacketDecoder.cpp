/*
 * PacketDecoder.cpp
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#include "PacketDecoder.h"

PacketDecoder::PacketDecoder()
{
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

bool PacketDecoder::DecodePacket(MicronetPacket_t *packet, WindTransducer_Message_t *message)
{
	int packetLength = packet->data[11];

	unsigned char crc = 0;
	for (int i = 0; i < packetLength; i++)
	{
		crc += packet->data[i];
	}

	if (crc != packet->data[packetLength]) {
		Serial.print("CRC FAILED : ");
		Serial.print(crc);
		Serial.print(" / ");
		Serial.println(packet->data[packetLength]);
	}

	short value;

	value = packet->data[16];
	value = (value << 8) | packet->data[17];
	message->windSpeedKt = ((float)value) / 10;

	value = packet->data[22];
	value = (value << 8) | packet->data[23];
	message->windAngleDeg = (float)value;

	Serial.print("WIND : ");
	Serial.print(message->windAngleDeg);
	Serial.print("° ");
	Serial.print(message->windSpeedKt);
	Serial.println("kt");

	return true;
}
