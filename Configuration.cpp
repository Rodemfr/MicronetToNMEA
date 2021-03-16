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
	uint16_t checksum;
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
}

Configuration::~Configuration()
{
}

void Configuration::LoadFromEeprom()
{
	ConfigBlock_t configBlock;
	uint16_t *pShortConfig = (uint16_t*) (&configBlock);

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
		}
	}
}

void Configuration::SaveToEeprom()
{
	ConfigBlock_t configBlock;
	uint16_t *pShortConfig = (uint16_t*) (&configBlock);
	uint16_t checksum = 0;

	configBlock.magicWord = CONFIG_MAGIC_NUMBER;
	configBlock.serialSpeed = serialSpeed;
	configBlock.attachedNetworkId = attachedNetworkId;
	for (uint32_t i = 0; i < (sizeof(ConfigBlock_t) - 2) / 2; i++)
	{
		checksum += pShortConfig[i];
	}
	configBlock.checksum = checksum;

	eeprom_write_block(&configBlock, EEPROM_CONFIG_OFFSET, sizeof(ConfigBlock_t));
}