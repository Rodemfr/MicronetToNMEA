/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Manage persistent configuration storage                        *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements persistent configuration storage using EEPROM.    *
 * It manages:                                                             *
 * - Default configuration values                                          *
 * - Loading/saving configuration blocks                                   *
 * - Configuration validation (magic word, checksum)                       *
 * - Type-safe configuration access                                        *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021-2025 Ronan Demoment                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "Configuration.h"
#include "BoardConfig.h"
#include "Globals.h"

#include <cstring>

extern "C"
{
#include "esp_random.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
}

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/**
 * Configuration storage constants
 * CONFIG_MAGIC_NUMBER: Identifier for valid config blocks
 */
#define CONFIG_NVS_NAMESPACE "config"
#define CONFIG_NVS_KEY       "block"
#define CONFIG_MAGIC_NUMBER  0x54495441 //'TITA'

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/**
 * Configuration block structure
 * Packed to ensure consistent EEPROM layout
 */
#pragma pack(1)
typedef struct
{
    uint32_t       magicWord; // Config block identifier
    EEPROMConfig_t config;    // User configuration data
    uint8_t        checksum;  // Simple checksum for validation
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

/**
 * Configuration class constructor
 *
 * Initializes:
 * - Configuration validation flags
 * - Default values for all settings
 * - Device unique identifier
 * - Sensor calibration values
 * - Data source routing
 * - Navigation parameters
 */
Configuration::Configuration() : magicNumberFound(false), checksumValid(false), eepromInit(false)
{
    // Set default configuration
    memset(&eeprom, 0, sizeof(eeprom));
    ram.navCompassAvailable         = false;
    eeprom.networkId                = 0;
    eeprom.waterSpeedFactor_per     = 1.0f;
    eeprom.waterTemperatureOffset_C = 0;
    eeprom.depthOffset_m            = 0;
    eeprom.windSpeedFactor_per      = 1.0f;
    eeprom.windDirectionOffset_deg  = 0;
    eeprom.headingOffset_deg        = 0;
    eeprom.magneticVariation_deg    = 0;
    eeprom.windShift                = 10;
    eeprom.deviceId                 = (0x03100000 | (esp_random() & 0x000ffff8));
    eeprom.xMagOffset               = 0;
    eeprom.yMagOffset               = 0;
    eeprom.zMagOffset               = 0;
    eeprom.rfFrequencyOffset_MHz    = 0;

    eeprom.gnssSource    = LINK_GNSS;
    eeprom.windSource    = LINK_MICRONET;
    eeprom.depthSource   = LINK_MICRONET;
    eeprom.speedSource   = LINK_MICRONET;
    eeprom.compassSource = LINK_COMPASS;

    eeprom.sogCogFilteringEnable = false;
    eeprom.sogCogFilterLength    = 4;
    eeprom.spdEmulation          = false;

    eeprom.headingAxis = AXIS_MINUS_Z;
    eeprom.downAxis    = AXIS_MINUS_X;
}

/**
 * @brief Destroy the Configuration object.
 *
 * Perform any required cleanup. Currently no runtime resources need explicit
 * release.
 */
Configuration::~Configuration()
{
}

/**
 * @brief Load configuration from EEPROM.
 *
 * Reads a configuration block from EEPROM, checks the magic word and checksum,
 * and if valid copies persisted values into the runtime configuration
 * structure used by the application.
 */
void Configuration::LoadFromEeprom()
{
    ConfigBlock_t configBlock;

    memset(&configBlock, 0, sizeof(configBlock));

    InitEeprom();

    nvs_handle_t nvsHandle;
    if (nvs_open(CONFIG_NVS_NAMESPACE, NVS_READONLY, &nvsHandle) != ESP_OK)
    {
        return;
    }

    size_t    configBlockSize = sizeof(configBlock);
    esp_err_t ret             = nvs_get_blob(nvsHandle, CONFIG_NVS_KEY, &configBlock, &configBlockSize);
    nvs_close(nvsHandle);
    if (ret != ESP_OK || configBlockSize != sizeof(configBlock))
    {
        return;
    }

    uint8_t *pConfig = (uint8_t *)(&configBlock);

    if (configBlock.magicWord == CONFIG_MAGIC_NUMBER)
    {
        magicNumberFound = true;

        uint8_t checksum = 0;
        for (uint32_t i = 0; i < (sizeof(ConfigBlock_t) - 1); i++)
        {
            checksum += pConfig[i];
        }

        if (checksum == configBlock.checksum)
        {
            checksumValid = true;

            eeprom.networkId                = configBlock.config.networkId;
            eeprom.deviceId                 = configBlock.config.deviceId;
            eeprom.waterSpeedFactor_per     = configBlock.config.waterSpeedFactor_per;
            eeprom.waterTemperatureOffset_C = configBlock.config.waterTemperatureOffset_C;
            eeprom.depthOffset_m            = configBlock.config.depthOffset_m;
            eeprom.windSpeedFactor_per      = configBlock.config.windSpeedFactor_per;
            eeprom.windDirectionOffset_deg  = configBlock.config.windDirectionOffset_deg;
            eeprom.headingOffset_deg        = configBlock.config.headingOffset_deg;
            eeprom.magneticVariation_deg    = configBlock.config.magneticVariation_deg;
            eeprom.windShift                = configBlock.config.windShift;
            eeprom.xMagOffset               = configBlock.config.xMagOffset;
            eeprom.yMagOffset               = configBlock.config.yMagOffset;
            eeprom.zMagOffset               = configBlock.config.zMagOffset;
            eeprom.rfFrequencyOffset_MHz    = configBlock.config.rfFrequencyOffset_MHz;
            eeprom.gnssSource               = (LinkId_t)configBlock.config.gnssSource;
            eeprom.windSource               = (LinkId_t)configBlock.config.windSource;
            eeprom.depthSource              = (LinkId_t)configBlock.config.depthSource;
            eeprom.speedSource              = (LinkId_t)configBlock.config.speedSource;
            eeprom.compassSource            = (LinkId_t)configBlock.config.compassSource;
            eeprom.sogCogFilteringEnable    = configBlock.config.sogCogFilteringEnable;
            eeprom.sogCogFilterLength       = configBlock.config.sogCogFilterLength;
            if (eeprom.sogCogFilterLength >= SOG_COG_MAX_FILTERING_DEPTH)
            {
                eeprom.sogCogFilterLength = SOG_COG_MAX_FILTERING_DEPTH - 1;
            }
            eeprom.spdEmulation = configBlock.config.spdEmulation;
            eeprom.headingAxis  = (Axis_t)configBlock.config.headingAxis;
            eeprom.downAxis     = (Axis_t)configBlock.config.downAxis;
        }
    }
}

/**
 * @brief Save current configuration to EEPROM.
 *
 * Builds a configuration block with a magic word and checksum and writes it to
 * EEPROM only if the stored block differs from the current one.
 */
void Configuration::SaveToEeprom()
{
    ConfigBlock_t eepromBlock;
    ConfigBlock_t configBlock;

    memset(&eepromBlock, 0, sizeof(eepromBlock));
    memset(&configBlock, 0, sizeof(configBlock));

    InitEeprom();

    uint8_t *pEepromBlock = (uint8_t *)(&eepromBlock);
    uint8_t *pConfig      = (uint8_t *)(&configBlock);
    uint8_t  checksum     = 0;

    nvs_handle_t nvsHandle;
    if (nvs_open(CONFIG_NVS_NAMESPACE, NVS_READONLY, &nvsHandle) == ESP_OK)
    {
        size_t eepromBlockSize = sizeof(eepromBlock);
        if (nvs_get_blob(nvsHandle, CONFIG_NVS_KEY, &eepromBlock, &eepromBlockSize) != ESP_OK || eepromBlockSize != sizeof(eepromBlock))
        {
            memset(&eepromBlock, 0, sizeof(eepromBlock));
        }
        nvs_close(nvsHandle);
    }

    configBlock.magicWord                       = CONFIG_MAGIC_NUMBER;
    configBlock.config.networkId                = eeprom.networkId;
    configBlock.config.deviceId                 = eeprom.deviceId;
    configBlock.config.waterSpeedFactor_per     = eeprom.waterSpeedFactor_per;
    configBlock.config.waterTemperatureOffset_C = eeprom.waterTemperatureOffset_C;
    configBlock.config.depthOffset_m            = eeprom.depthOffset_m;
    configBlock.config.windSpeedFactor_per      = eeprom.windSpeedFactor_per;
    configBlock.config.windDirectionOffset_deg  = eeprom.windDirectionOffset_deg;
    configBlock.config.headingOffset_deg        = eeprom.headingOffset_deg;
    configBlock.config.magneticVariation_deg    = eeprom.magneticVariation_deg;
    configBlock.config.windShift                = eeprom.windShift;
    configBlock.config.xMagOffset               = eeprom.xMagOffset;
    configBlock.config.yMagOffset               = eeprom.yMagOffset;
    configBlock.config.zMagOffset               = eeprom.zMagOffset;
    configBlock.config.rfFrequencyOffset_MHz    = eeprom.rfFrequencyOffset_MHz;
    configBlock.config.gnssSource               = (uint8_t)eeprom.gnssSource;
    configBlock.config.windSource               = (uint8_t)eeprom.windSource;
    configBlock.config.depthSource              = (uint8_t)eeprom.depthSource;
    configBlock.config.speedSource              = (uint8_t)eeprom.speedSource;
    configBlock.config.compassSource            = (uint8_t)eeprom.compassSource;
    configBlock.config.sogCogFilteringEnable    = eeprom.sogCogFilteringEnable;
    configBlock.config.sogCogFilterLength       = eeprom.sogCogFilterLength;
    configBlock.config.spdEmulation             = eeprom.spdEmulation;
    configBlock.config.headingAxis              = eeprom.headingAxis;
    configBlock.config.downAxis                 = eeprom.downAxis;

    for (uint32_t i = 0; i < sizeof(ConfigBlock_t) - 1; i++)
    {
        checksum += pConfig[i];
    }
    configBlock.checksum = checksum;

    for (uint32_t i = 0; i < sizeof(ConfigBlock_t); i++)
    {
        if (pEepromBlock[i] != pConfig[i])
        {
            if (nvs_open(CONFIG_NVS_NAMESPACE, NVS_READWRITE, &nvsHandle) == ESP_OK)
            {
                if (nvs_set_blob(nvsHandle, CONFIG_NVS_KEY, &configBlock, sizeof(configBlock)) == ESP_OK)
                {
                    (void)nvs_commit(nvsHandle);
                }
                nvs_close(nvsHandle);
            }
            break;
        }
    }
}

/**
 * @brief Initialize the NVS subsystem if necessary.
 *
 * Ensures NVS has been initialized before any read or write operations are
 * performed. NVS initialization is idempotent when another component already
 * initialized the flash partition.
 */
void Configuration::InitEeprom()
{
    if (!eepromInit)
    {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            (void)nvs_flash_erase();
            ret = nvs_flash_init();
        }
        eepromInit = (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE);
    }
}