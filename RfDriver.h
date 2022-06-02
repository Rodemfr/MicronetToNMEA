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
#include "CC1101Driver.h"

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

	bool Init(MicronetMessageFifo *messageFifo, float frequencyOffset_mHz);
	void SetFrequencyOffset(float offsetMHz);
	void SetFrequency(float freqMHz);
	void SetDeviation(float freqKHz);
	void SetBandwidth(float bwKHz);
	void SetBaudrateOffset(float offset_baud);
	void SetBaudrate(float baudrate_baud);
	void GDO0Callback();
	void RestartReception();
	void TransmitMessage(MicronetMessage_t *message, uint32_t transmitTimeUs);

private:
	CC1101Driver cc1101Driver;
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
