/*
 * PacketStore.h
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#ifndef MICRONETMESSAGEFIFO_H_
#define MICRONETMESSAGEFIFO_H_

#include "Micronet.h"
#include <arduino.h>
#include <stdint.h>

#define MESSAGE_STORE_SIZE 4
class MicronetMessageFifo
{
public:
	MicronetMessageFifo();
	virtual ~MicronetMessageFifo();

	bool Push(MicronetMessage_t &message);
	bool Pop(MicronetMessage_t *message);
	MicronetMessage_t *Peek();
	void DeleteMessage();

private:
	int writeIndex;
	int readIndex;
	int nbMessages;
	MicronetMessage_t store[MESSAGE_STORE_SIZE];
};

#endif /* MICRONETMESSAGEFIFO_H_ */
