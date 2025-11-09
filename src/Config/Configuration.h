/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Manage persistent and runtime configuration                    *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module provides centralized configuration management:               *
 * - EEPROM-based persistent storage                                       *
 * - Runtime configuration access                                          *
 * - Type-safe configuration structures                                    *
 * - Automatic validation (magic word, checksum)                           *
 *                                                                         *
 * Configuration includes:                                                 *
 * - Network and device identifiers                                        *
 * - Sensor calibration values                                            *
 * - Data routing preferences                                             *
 * - Navigation parameters                                                 *
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

#pragma once

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include <stdint.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/**
 * Maximum depth for SOG/COG filtering
 * Larger values provide more smoothing but increase latency
 */
#define SOG_COG_MAX_FILTERING_DEPTH 16

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/**
 * Radio frequency system selection
 * Used to configure RF hardware for regional frequency plans
 */
typedef enum
{
    RF_FREQ_SYSTEM_868 = 0,  ///< European 868 MHz band
    RF_FREQ_SYSTEM_915       ///< North American 915 MHz band
} FreqSystem_t;

/**
 * Data link identifiers
 * Used to route data between different physical and logical sources
 */
typedef enum
{
    LINK_PLOTTER = 0,     ///< External chart plotter
    LINK_NMEA0183_IN,     ///< NMEA0183 input port
    LINK_MICRONET,        ///< Micronet wireless network
    LINK_COMPASS          ///< Internal compass sensor
} LinkId_t;

/**
 * Axis identifiers for sensor orientation
 * Defines possible mounting orientations for magnetic sensors
 */
typedef enum
{
    AXIS_X = 0,           ///< Positive X axis
    AXIS_MINUS_X,         ///< Negative X axis  
    AXIS_Y,              ///< Positive Y axis
    AXIS_MINUS_Y,         ///< Negative Y axis
    AXIS_Z,              ///< Positive Z axis
    AXIS_MINUS_Z          ///< Negative Z axis
} Axis_t;

/**
 * EEPROM-stored configuration
 * All fields persisted across power cycles
 */
typedef struct
{
    uint32_t networkId;                  ///< Micronet network identifier
    uint32_t deviceId;                   ///< Device unique identifier
    float    waterSpeedFactor_per;       ///< Water speed calibration factor
    float    waterTemperatureOffset_C;   ///< Water temp sensor offset
    float    depthOffset_m;              ///< Depth sensor offset
    float    windSpeedFactor_per;        ///< Wind speed calibration factor
    float    windDirectionOffset_deg;    ///< Wind direction offset
    float    headingOffset_deg;          ///< Magnetic heading offset
    float    magneticVariation_deg;      ///< Local magnetic variation
    float    windShift;                  ///< Wind shift angle
    float    xMagOffset;                 ///< X-axis magnetometer offset
    float    yMagOffset;                 ///< Y-axis magnetometer offset
    float    zMagOffset;                 ///< Z-axis magnetometer offset
    float    rfFrequencyOffset_MHz;      ///< RF crystal frequency trim
    uint8_t  gnssSource;                 ///< GNSS data source selection
    uint8_t  windSource;                 ///< Wind data source selection
    uint8_t  depthSource;                ///< Depth data source selection
    uint8_t  speedSource;                ///< Speed data source selection
    uint8_t  compassSource;              ///< Compass data source selection
    uint8_t  sogCogFilteringEnable;      ///< SOG/COG filter enable flag
    uint8_t  sogCogFilterLength;         ///< SOG/COG filter depth
    uint8_t  spdEmulation;               ///< Speed emulation enable
    Axis_t   headingAxis;                ///< Heading reference axis
    Axis_t   downAxis;                   ///< Vertical reference axis
} EEPROMConfig_t;

/**
 * Runtime-only configuration
 * Values not persisted to EEPROM
 */
typedef struct
{
    bool navCompassAvailable;            ///< Compass hardware detected flag
} RAMConfig_t;

/***************************************************************************/
/*                              Classes                                    */
/***************************************************************************/

/**
 * Configuration
 *
 * Central configuration management class providing:
 * - EEPROM initialization and access
 * - Configuration load/save operations
 * - Status tracking (magic number, checksum, init state)
 * - Access to both persistent and runtime configuration
 */
class Configuration
{
  public:
    /**
     * Constructor
     * Initializes configuration with default values
     */
    Configuration();

    /**
     * Virtual destructor
     */
    virtual ~Configuration();

    /**
     * Load configuration from EEPROM
     * Verifies magic number and checksum before accepting
     */
    void LoadFromEeprom();

    /**
     * Save current configuration to EEPROM
     * Only writes if different from stored values
     */
    void SaveToEeprom();

    /**
     * Initialize EEPROM subsystem
     * Safe to call multiple times
     */
    void InitEeprom();

    bool magicNumberFound;              ///< Valid config block found flag
    bool checksumValid;                 ///< Config checksum valid flag
    bool eepromInit;                    ///< EEPROM initialized flag

    EEPROMConfig_t eeprom;             ///< Persistent configuration
    RAMConfig_t    ram;                ///< Runtime configuration
};
