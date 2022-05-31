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

#include <SPI.h>

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

	bool Init(int gdo0_pin, int sckPin, int misoPin, int mosiPin, int csPin, MicronetMessageFifo *messageFifo, float frequencyOffset_mHz);
	void SetFrequencyOffset(float offsetMHz);
	void SetFrequency(float freqMHz);
	void GDO0Callback();
	void RestartReception();
	void TransmitMessage(MicronetMessage_t *message, uint32_t transmitTimeUs);

	void DebugPrintRegs();

private:
	int gdo0Pin;
	int sckPin, mosiPin, misoPin, csPin;
	bool spiInit;
	SPISettings spiSettings;
	MicronetMessageFifo *messageFifo;
	RfDriverState_t rfState;
	MicronetMessage_t messageToTransmit;
	int messageBytesSent;
	float frequencyOffset_mHz;
	static RfDriver *rfDriver;

	void GDO0RxCallback();
	void GDO0TxCallback();
	void GDO0LastTxCallback();
	void TransmitCallback();
	static void TimerHandler();

	void SpiWriteReg(uint8_t addr, uint8_t value);
	void SpiWriteBurstReg(uint8_t addr, uint8_t *buffer, uint8_t nbBytes);
	void SpiStrobe(uint8_t strobe);
	uint8_t SpiReadReg(uint8_t addr);
	void SpiReadBurstReg(uint8_t addr, uint8_t *buffer, uint8_t nbBytes);
	uint8_t SpiReadStatus(uint8_t addr);

	int GetRssi();
	void SetIdle();
	void SetRx();
	void SetTx();
	void ResetRxFifo();
	void ResetTxFifo();
};

#endif /* RFDRIVER_H_ */
