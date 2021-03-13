/*
 * PacketStore.h
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#ifndef PACKETSTORE_H_
#define PACKETSTORE_H_

#include <arduino.h>
#include <stdint.h>

#define PACKET_STORE_SIZE 4
#define MESSAGE_MAX_LENGTH 80
#define MICRONET_MESSAGE_MIN_LENGTH 13

typedef struct
{
	uint8_t len;
	int16_t rssi;
	uint8_t lqi;
	uint8_t data[MESSAGE_MAX_LENGTH];
} MicronetPacket_t;

class PacketStore
{
public:
	PacketStore();
	virtual ~PacketStore();

	bool AddPacket(MicronetPacket_t &packet);
	bool GetPacket(MicronetPacket_t *packet);
	MicronetPacket_t *PeekPacket();
	void Deletepacket();

private:
	int writeIndex;
	int readIndex;
	int nbPackets;
	MicronetPacket_t store[PACKET_STORE_SIZE];
};

#endif /* PACKETSTORE_H_ */
