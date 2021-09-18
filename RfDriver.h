/*
 * RfDriver.h
 *
 *  Created on: 18 sept. 2021
 *      Author: Ronan
 */

#ifndef RFDRIVER_H_
#define RFDRIVER_H_

#include "Micronet.h"
#include "MicronetMessageFifo.h"
#include "ELECHOUSE_CC1101_SRC_DRV.h"

class RfDriver
{
public:
	RfDriver();
	virtual ~RfDriver();

	bool Init(int gdo0_pin, int gdo2_pin, MicronetMessageFifo *messageFifo);
	void Gdo0Isr();
	void RfFlushAndRestartRx();
	void RfTxMessage(MicronetMessage_t *message);

private:
	int gdo0_pin;
	int gdo2_pin;
	ELECHOUSE_CC1101 RfReceiver;
	MicronetMessageFifo *messageFifo;
};

#endif /* RFDRIVER_H_ */
