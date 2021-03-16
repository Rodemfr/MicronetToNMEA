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

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "Configuration.h"

#include <Arduino.h>
#include <avr/eeprom.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define EEPROM_CONFIG_OFFSET 0
#define CONFIG_MAGIC_NUMBER  0x4D544E4D

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

#pragma pack(1)
typedef struct
{
	uint32_t magicWord;
	uint32_t serialSpeed;
	uint32_t attachedNetworkId;
	float waterSpeedFactor_per;
	float waterTemperatureOffset_C;
	float distanceToDepthTransducer_m;
	int16_t windDirectionOffset_deg;
	int16_t headingDirectionOffset_deg;
	int16_t magneticVariation_deg;
	int16_t windShift;
	uint8_t checksum;
} ConfigBlock_t;
#pragma pack()

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

Configuration::Configuration()
{
	// Set default configuration
	serialSpeed = 4800;
	attachedNetworkId = 0;
	waterSpeedFactor_per = 0;
	waterTemperatureOffset_C = 0;
	distanceToDepthTransducer_m = 0;
	windDirectionOffset_deg = 0;
	headingDirectionOffset_deg = 0;
	magneticVariation_deg = 0;
	windShift = 0;
}

Configuration::~Configuration()
{
}

void Configuration::LoadFromEeprom()
{
	ConfigBlock_t configBlock;
	uint16_t *pShortConfig = (uint16_t*) (&configBlock);

	memset(&configBlock, 0, sizeof(configBlock));

	eeprom_read_block(&configBlock, EEPROM_CONFIG_OFFSET, sizeof(ConfigBlock_t));

	if (configBlock.magicWord == CONFIG_MAGIC_NUMBER)
	{
		uint16_t checksum = 0;
		for (uint32_t i = 0; i < (sizeof(ConfigBlock_t) - 2) / 2; i++)
		{
			checksum += pShortConfig[i];
		}

		if (checksum == configBlock.checksum)
		{
			serialSpeed = configBlock.serialSpeed;
			attachedNetworkId = configBlock.attachedNetworkId;
			waterSpeedFactor_per = configBlock.waterSpeedFactor_per;
			waterTemperatureOffset_C = configBlock.waterTemperatureOffset_C;
			distanceToDepthTransducer_m = configBlock.distanceToDepthTransducer_m;
			windDirectionOffset_deg = configBlock.windDirectionOffset_deg;
			headingDirectionOffset_deg = configBlock.headingDirectionOffset_deg;
			magneticVariation_deg = configBlock.magneticVariation_deg;
			windShift = configBlock.windShift;
		}
	}
}

void Configuration::SaveToEeprom()
{
	ConfigBlock_t configBlock;
	uint16_t *pShortConfig = (uint16_t*) (&configBlock);
	uint8_t checksum = 0;

	configBlock.magicWord = CONFIG_MAGIC_NUMBER;
	configBlock.serialSpeed = serialSpeed;
	configBlock.attachedNetworkId = attachedNetworkId;
	configBlock.waterSpeedFactor_per = waterSpeedFactor_per;
	configBlock.waterTemperatureOffset_C = waterTemperatureOffset_C;
	configBlock.distanceToDepthTransducer_m = distanceToDepthTransducer_m;
	configBlock.windDirectionOffset_deg = windDirectionOffset_deg;
	configBlock.headingDirectionOffset_deg = headingDirectionOffset_deg;
	configBlock.magneticVariation_deg = magneticVariation_deg;
	configBlock.windShift = windShift;

	for (uint32_t i = 0; i < sizeof(ConfigBlock_t) - 1; i++)
	{
		checksum += pShortConfig[i];
	}
	configBlock.checksum = checksum;

	eeprom_write_block(&configBlock, EEPROM_CONFIG_OFFSET, sizeof(ConfigBlock_t));
}
