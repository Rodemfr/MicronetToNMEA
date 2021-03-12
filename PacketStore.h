/*
 * PacketStore.h
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#ifndef PACKETSTORE_H_
#define PACKETSTORE_H_

#include <arduino.h>

#define PACKET_STORE_SIZE 4

typedef struct
{
	unsigned char len;
	signed int rssi;
	unsigned char lqi;
	unsigned char crcOk;
	unsigned char data[64];
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
