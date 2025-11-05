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

#pragma once

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include <stdint.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

// Maximum depth of COG/SOG filter
#define SOG_COG_MAX_FILTERING_DEPTH 16

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/**
 * @brief Radio frequency system selection used for RF configuration.
 *
 * Indicates which regional RF frequency plan the device should use (e.g. 868 MHz
 * or 915 MHz).
 */
typedef enum
{
    RF_FREQ_SYSTEM_868 = 0,
    RF_FREQ_SYSTEM_915
} FreqSystem_t;

/**
 * @brief Identifiers for logical data links/sources.
 *
 * Used to select or identify the source or destination of navigation and
 * sensor data within the application (plotter, GNSS, Micronet devices,
 * compass, or the legacy AIS/NMEA link).
 */
typedef enum
{
    LINK_PLOTTER = 0,
    LINK_GNSS,
    LINK_MICRONET,
    LINK_COMPASS,
    LINK_AIS
} LinkId_t;

/**
 * @brief Axis identifiers for sensor mounting orientation.
 *
 * Enumerates the possible axes (positive and negative directions) that can be
 * used to describe magnetic sensor orientation relative to the device chassis.
 */
typedef enum
{
    AXIS_X = 0,
    AXIS_MINUS_X,
    AXIS_Y,
    AXIS_MINUS_Y,
    AXIS_Z,
    AXIS_MINUS_Z
} Axis_t;

/**
 * @brief Persistent configuration stored in EEPROM.
 *
 * This structure contains all configuration values that are saved to and
 * restored from EEPROM, including identifiers, scaling/calibration factors,
 * data source selections, filtering options and axis mappings.
 */
typedef struct
{
    uint32_t networkId;
    uint32_t deviceId;
    float    waterSpeedFactor_per;
    float    waterTemperatureOffset_C;
    float    depthOffset_m;
    float    windSpeedFactor_per;
    float    windDirectionOffset_deg;
    float    headingOffset_deg;
    float    magneticVariation_deg;
    float    windShift;
    float    xMagOffset;
    float    yMagOffset;
    float    zMagOffset;
    float    rfFrequencyOffset_MHz;
    uint8_t  gnssSource;
    uint8_t  windSource;
    uint8_t  depthSource;
    uint8_t  speedSource;
    uint8_t  compassSource;
    uint8_t  sogCogFilteringEnable;
    uint8_t  sogCogFilterLength;
    uint8_t  spdEmulation;
    Axis_t   headingAxis;
    Axis_t   downAxis;
} EEPROMConfig_t;

/**
 * @brief Runtime-only (non-persistent) configuration.
 *
 * Contains flags and state used at runtime that are not saved to EEPROM.
 */
typedef struct
{
    bool navCompassAvailable;
} RAMConfig_t;

/**
 * @class Configuration
 * @brief Central access point for application configuration and persistence.
 *
 * The Configuration class provides simple access to the application's
 * configuration data. It exposes methods to initialize EEPROM access,
 * load persisted settings into RAM, and save current settings back to EEPROM.
 * Public members include status flags (magicNumberFound, checksumValid,
 * eepromInit) and the eeprom/ram structures representing persistent and
 * runtime configuration respectively.
 */
class Configuration
{
  public:
    Configuration();
    virtual ~Configuration();

    void LoadFromEeprom();
    void SaveToEeprom();
    void InitEeprom();

    bool magicNumberFound;
    bool checksumValid;
    bool eepromInit;

    // The following parameters are loaded/saved from/to EEPROM
    EEPROMConfig_t eeprom;
    // The following parameters are NOT loaded/saved from/to EEPROM
    RAMConfig_t ram;
};

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/
