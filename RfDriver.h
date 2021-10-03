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
	RF_STATE_RX_IDLE = 0,
	RF_STATE_RX_RECEIVING,
	RF_STATE_TX_TRANSMITTING,
	RF_STATE_TX_LAST_TRANSMIT
} RfDriverState_t;

class RfDriver
{
public:
	RfDriver();
	virtual ~RfDriver();

	bool Init(int gdo0_pin, MicronetMessageFifo *messageFifo, float frequencyOffset_mHz);
	void SetFrequencyOffset(float offsetMHz);
	void SetFrequency(float freqMhz);
	void SetDeviation(float freqMhz);
	void SetBandwidth(float bwMHz);
	void GDO0Callback();
	void RestartReception();
	void TransmitMessage(MicronetMessage_t *message, uint32_t transmitTimeUs);

private:
	int gdo0Pin;
	ELECHOUSE_CC1101 cc1101Driver;
	MicronetMessageFifo *messageFifo;
	RfDriverState_t rfState;
	MicronetMessage_t messageToTransmit;
	int messageBytesSent;
	float frequencyOffset_mHz;

	void GDO0RxCallback();
	void GDO0TxCallback();
	void GDO0LastTxCallback();
	void TransmitCallback();
	static void TimerHandler();
	static RfDriver *rfDriver;
};

#endif /* RFDRIVER_H_ */
