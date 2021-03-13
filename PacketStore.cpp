/*
 * PacketStore.cpp
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#include "PacketStore.h"

PacketStore::PacketStore()
{
	// Reset packet store
	writeIndex = 0;
	readIndex = 0;
	nbPackets = 0;
}

PacketStore::~PacketStore()
{
}

bool PacketStore::AddPacket(MicronetPacket_t &packet)
{
	// Disable interrupts to avoid race conditions
	noInterrupts();
	// Check if there is space in store. If not, the packet is just dropped/ignored.
	if (nbPackets < MESSAGE_STORE_SIZE)
	{
		// Yes : copy packet to the store and update store's status
		memcpy(&(store[writeIndex]), &packet, sizeof(packet));
		writeIndex++;
		nbPackets++;
		if (writeIndex >= MESSAGE_STORE_SIZE)
		{
			writeIndex = 0;
		}
	}
	else
	{
		interrupts();
		return false;
	}
	interrupts();

	return true;
}

bool PacketStore::GetPacket(MicronetPacket_t *packet)
{
	// Disable interrupts to avoid race conditions
	noInterrupts();

	// Are there packets in the store ?
	if (nbPackets > 0)
	{
		// Copy packet
		memcpy(packet, &(store[readIndex]), sizeof(MicronetPacket_t));
		// Remove packet from the store
		readIndex++;
		if (readIndex >= MESSAGE_STORE_SIZE)
		{
			readIndex = 0;
		}
		nbPackets--;
	}
	else
	{
		interrupts();
		return false;
	}
	interrupts();

	return true;
}

MicronetPacket_t *PacketStore::PeekPacket()
{
	MicronetPacket_t *pPacket = nullptr;

	// Disable interrupts to avoid race conditions
	noInterrupts();

	// Are there packets in the store ?
	if (nbPackets > 0)
	{
		pPacket = &(store[readIndex]);
	}

	interrupts();

	return pPacket;
}

void PacketStore::Deletepacket()
{
	noInterrupts();

	// Are there packets in the store ?
	if (nbPackets > 0)
	{
		// Yes : delete the next one
		readIndex++;
		if (readIndex >= MESSAGE_STORE_SIZE)
		{
			readIndex = 0;
		}
		nbPackets--;
	}

	interrupts();
}
