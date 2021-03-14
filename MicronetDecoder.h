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
	bool valid;
	bool updated;
	float value;
	uint32_t timeStamp;
} DataValue_t;

typedef struct {
	DataValue_t stw;
	DataValue_t awa;
	DataValue_t aws;
	DataValue_t dpt;
	DataValue_t vcc;
	DataValue_t log;
	DataValue_t trip;
	DataValue_t stp;
} MicronetData_t;

class MicronetDecoder
{
public:
	MicronetDecoder();
	virtual ~MicronetDecoder();

	uint32_t GetNetworkId(MicronetMessage_t *message);
	uint8_t GetDeviceType(MicronetMessage_t *message);
	uint32_t GetDeviceId(MicronetMessage_t *message);
	uint8_t GetMessageId(MicronetMessage_t *message);
	uint8_t GetSource(MicronetMessage_t *message);
	uint8_t GetDestination(MicronetMessage_t *message);
	uint8_t GetHeaderCrc(MicronetMessage_t *message);
	bool VerifyHeaderCrc(MicronetMessage_t *message);
	void DecodeMessage(MicronetMessage_t *message);
	void PrintRawMessage(MicronetMessage_t *message);
	void PrintCurrentData();
	MicronetData_t *GetCurrentData();

private:
	MicronetData_t micronetData;
	int DecodeDataField(MicronetMessage_t *message, int offset);
	void UpdateMicronetData(uint8_t fieldId, int8_t value);
	void UpdateMicronetData(uint8_t fieldId, int16_t value);
	void UpdateMicronetData(uint8_t fieldId, int32_t value1, int32_t value2);
};

#endif /* MICRONETDECODER_H_ */
