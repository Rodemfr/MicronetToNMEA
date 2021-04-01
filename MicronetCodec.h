/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Decode data from Micronet devices send it on an NMEA network  *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021 by Ronan Demoment                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#ifndef MICRONETCODEC_H_
#define MICRONETCODEC_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include <stdint.h>

#include "Micronet.h"
#include "NmeaDecoder.h"
#include "NavigationData.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define DEVICE_TYPE_HULL_TRANSMITTER    0x01
#define DEVICE_TYPE_WIND_TRANSDUCER     0x02
#define DEVICE_TYPE_DUAL_DISPLAY        0x81
#define DEVICE_TYPE_ANALOG_WIND_DISPLAY 0x83

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

class MicronetCodec
{
public:
	MicronetCodec();
	virtual ~MicronetCodec();

	uint32_t GetNetworkId(MicronetMessage_t *message);
	uint8_t GetDeviceType(MicronetMessage_t *message);
	uint32_t GetDeviceId(MicronetMessage_t *message);
	uint8_t GetMessageId(MicronetMessage_t *message);
	uint8_t GetSource(MicronetMessage_t *message);
	uint8_t GetDestination(MicronetMessage_t *message);
	uint8_t GetHeaderCrc(MicronetMessage_t *message);
	bool VerifyHeaderCrc(MicronetMessage_t *message);

	void DecodeMessage(MicronetMessage_t *message, NavigationData *dataSet);
	uint32_t GetTransmissionSlot(MicronetMessage_t *message);
	bool BuildGnssMessage(MicronetMessage_t *message, uint32_t networkId, NmeaData_t *nmeaData);

private:
	void DecodeSendDataMessage(MicronetMessage_t *message, NavigationData *dataSet);
	void DecodeSetParameterMessage(MicronetMessage_t *message, NavigationData *dataSet);
	int DecodeDataField(MicronetMessage_t *message, int offset, NavigationData *dataSet);
	void UpdateMicronetData(uint8_t fieldId, int8_t value, NavigationData *dataSet);
	void UpdateMicronetData(uint8_t fieldId, int16_t value, NavigationData *dataSet);
	void UpdateMicronetData(uint8_t fieldId, int32_t value1, int32_t value2, NavigationData *dataSet);
	void CalculateTrueWind(NavigationData *dataSet);
	void WriteHeaderLengthAndCrc(MicronetMessage_t *message);
	uint8_t AddPositionField(uint8_t *buffer, float latitude, float longitude);
	uint8_t Add16bitField(uint8_t *buffer, uint8_t fieldCode, int16_t value);
	uint8_t AddDual16bitField(uint8_t *buffer, uint8_t fieldCode, int16_t value1, int16_t value2);
	uint8_t AddDual32bitField(uint8_t *buffer, uint8_t fieldCode, int32_t value1, int32_t value2);
	uint8_t Add24bitField(uint8_t *buffer, uint8_t fieldCode, int32_t value);
	uint8_t Add32bitField(uint8_t *buffer, uint8_t fieldCode, int32_t value);
};

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* MICRONETCODEC_H_ */
