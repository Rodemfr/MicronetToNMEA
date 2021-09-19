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

typedef enum {
	RX_STATE_IDLE = 0,
	RX_STATE_RECEIVING
} RfDriverRxState_t;

class RfDriver
{
public:
	RfDriver();
	virtual ~RfDriver();

	bool Init(int gdo0_pin, int gdo2_pin, MicronetMessageFifo *messageFifo);
	void GDO0Callback();
	void RestartReception();
	void TransmitMessage(MicronetMessage_t *message);

private:
	int gdo0Pin;
	int gdo2Pin;
	ELECHOUSE_CC1101 RfReceiver;
	MicronetMessageFifo *messageFifo;
	RfDriverRxState_t rxState;
};

#endif /* RFDRIVER_H_ */
