/*
 * PacketStore.cpp
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#include "MicronetMessageFifo.h"

MicronetMessageFifo::MicronetMessageFifo()
{
	// Reset packet store
	writeIndex = 0;
	readIndex = 0;
	nbMessages = 0;
}

MicronetMessageFifo::~MicronetMessageFifo()
{
}

bool MicronetMessageFifo::Push(MicronetMessage_t &message)
{
	// Disable interrupts to avoid race conditions
	noInterrupts();
	// Check if there is space in store. If not, the message is just dropped/ignored.
	if (nbMessages < MESSAGE_STORE_SIZE)
	{
		// Yes : copy message to the store and update store's status
		memcpy(&(store[writeIndex]), &message, sizeof(message));
		writeIndex++;
		nbMessages++;
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

bool MicronetMessageFifo::Pop(MicronetMessage_t *message)
{
	// Disable interrupts to avoid race conditions
	noInterrupts();

	// Are there messages in the store ?
	if (nbMessages > 0)
	{
		// Yes : Copy message
		memcpy(message, &(store[readIndex]), sizeof(MicronetMessage_t));
		// Remove message from the store
		readIndex++;
		if (readIndex >= MESSAGE_STORE_SIZE)
		{
			readIndex = 0;
		}
		nbMessages--;
	}
	else
	{
		interrupts();
		return false;
	}
	interrupts();

	return true;
}

MicronetMessage_t *MicronetMessageFifo::Peek()
{
	MicronetMessage_t *pMessage = nullptr;

	// Disable interrupts to avoid race conditions
	noInterrupts();

	// Are there messages in the store ?
	if (nbMessages > 0)
	{
		pMessage = &(store[readIndex]);
	}

	interrupts();

	return pMessage;
}

void MicronetMessageFifo::DeleteMessage()
{
	noInterrupts();

	// Are there messages in the store ?
	if (nbMessages > 0)
	{
		// Yes : delete the next one
		readIndex++;
		if (readIndex >= MESSAGE_STORE_SIZE)
		{
			readIndex = 0;
		}
		nbMessages--;
	}

	interrupts();
}

void MicronetMessageFifo::ResetFifo()
{
	noInterrupts();
	readIndex = writeIndex;
	nbMessages = 0;
	interrupts();
}
