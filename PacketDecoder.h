/*
 * PacketDecoder.h
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#ifndef PACKETDECODER_H_
#define PACKETDECODER_H_

#include "PacketStore.h"

#include <stdint.h>

#define DEVICE_TYPE_HULL_TRANSMITTER    0x01
#define DEVICE_TYPE_WIND_TRANSDUCER     0x02
#define DEVICE_TYPE_DUAL_DISPLAY        0x81
#define DEVICE_TYPE_ANALOG_WIND_DISPLAY 0x83

typedef enum {
	MSG_CATEGORY_WIND
} MessageCategory_t;

typedef struct {
	float windAngleDeg;
	float windSpeedKt;
} WindTransducer_Message_t;

class PacketDecoder
{
public:
	PacketDecoder();
	virtual ~PacketDecoder();

	uint32_t GetNetworkId(MicronetPacket_t *packet);
	uint8_t GetDeviceType(MicronetPacket_t *packet);
	uint32_t GetDeviceId(MicronetPacket_t *packet);
	uint8_t GetMessageCategory(MicronetPacket_t *packet);
	uint8_t GetMessageId(MicronetPacket_t *packet);
	bool DecodePacket(MicronetPacket_t *packet, WindTransducer_Message_t *message);
};

#endif /* PACKETDECODER_H_ */
