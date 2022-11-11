/*
 * M8NDriver.h
 *
 *  Created on: 13 mai 2022
 *      Author: Ronan
 *  Update on 20 july 2022
 * 
 */

#ifndef M8NDRIVER_H_
#define M8NDRIVER_H_

#include <Arduino.h>

#define M8N_GGA_ENABLE  0x00000001
#define M8N_VTG_ENABLE  0x00000002
#define M8N_RMC_ENABLE  0x00000004
#define M8N_HISPEED_NAV 0x00000008

class M8NDriver
{
public:
	M8NDriver();
	virtual ~M8NDriver();

	static const PROGMEM uint8_t ClearConfig[];
	static const PROGMEM uint8_t UART1_38400[];
	static const PROGMEM uint8_t Navrate5hz[];
	static const PROGMEM uint8_t GNSSSetup[];

	void Start(uint32_t nmeaSentences);

private:
	void GPS_SendConfig(const uint8_t *progmemPtr, uint8_t arraySize);
	void GPS_SendPUBX(const char pubxMsg[]);
};

#endif /* M8NDRIVER_H_ */
