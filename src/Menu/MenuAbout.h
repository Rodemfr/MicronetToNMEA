/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Provide "About" menu entry that reports device and runtime    *
 *           configuration status on the console.                          *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the "About" menu. The         *
 * implementation prints firmware version, EEPROM/persistent configuration *
 * status, detected hardware state and relevant calibration values to the  *
 * console for diagnostic purposes.                                         *
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

/* No public headers required by callers. Implementation includes project
   headers (Configuration, Globals, Version, etc.) */

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* No public constants */

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/* No public types */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuAbout
 *
 * Entry point invoked by the menu system when the user selects "About".
 * This function prints diagnostic information to the console including:
 *  - Software version
 *  - EEPROM / configuration validity and checksum status
 *  - Device and attached Micronet network identifiers
 *  - RF frequency offset and equivalent ppm
 *  - Calibration and offsets for wind, speed, temperature, depth, etc.
 *  - Compass detection state and magnetometer calibration if available
 *
 * The function is read-only: it reports information and does not modify state.
 */
void MenuAbout(void);