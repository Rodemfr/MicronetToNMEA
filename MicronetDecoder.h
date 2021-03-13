/*
 * PacketDecoder.h
 *
 *  Created on: 12 mars 2021
 *      Author: Ronan
 */

#ifndef MICRONETDECODER_H_
#define MICRONETDECODER_H_

#include "Micronet.h"

#include <stdint.h>

#define DEVICE_TYPE_HULL_TRANSMITTER    0x01
#define DEVICE_TYPE_WIND_TRANSDUCER     0x02
#define DEVICE_TYPE_DUAL_DISPLAY        0x81
#define DEVICE_TYPE_ANALOG_WIND_DISPLAY 0x83

typedef enum {
	MSG_CATEGORY_WIND
} MessageCategory_t;

typedef struct {
	bool awaValid;
	float awaDeg;
	bool awsValid;
	float awsKt;
	bool twaValid;
	float twaDeg;
	bool twsValid;
	float twsKt;
	bool depthValid;
	float depthM;
	bool stwValid;
	float stwKt;
	bool vccValid;
	float vccV;
} MicronetData_t;

typedef struct {
	uint32_t awaTimeStamp;
	uint32_t awsTimeStamp;
	uint32_t twaTimeStamp;
	uint32_t twsTimeStamp;
	uint32_t depthTimeStamp;
	uint32_t stwTimeStamp;
	uint32_t vccTimeStamp;
} DataTimeStamps_t;

class MicronetDecoder
{
public:
	MicronetDecoder();
	virtual ~MicronetDecoder();

	uint32_t GetNetworkId(MicronetMessage_t *message);
	uint8_t GetDeviceType(MicronetMessage_t *message);
	uint32_t GetDeviceId(MicronetMessage_t *message);
	uint8_t GetMessageCategory(MicronetMessage_t *message);
	uint8_t GetMessageId(MicronetMessage_t *message);
	void DecodeMessage(MicronetMessage_t *message);
	void PrintRawMessage(MicronetMessage_t *message);
	void PrintCurrentData();
	MicronetData_t *GetCurrentData();

private:
	MicronetData_t micronetData;
	DataTimeStamps_t dataTimeStamps;
	int DecodeDataField(MicronetMessage_t *message, int offset);
	void UpdateMicronetData(uint8_t fieldId, int16_t value);
};

#endif /* MICRONETDECODER_H_ */
