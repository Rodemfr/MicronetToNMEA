/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Interactive magnetometer calibration menu declaration         *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the magnetometer calibration   *
 * routine. The implementation samples the navigation magnetometer while   *
 * the user slowly rotates the device, computes per-axis minima/maxima    *
 * and proposes center offsets to be stored in persistent configuration.   *
 *                                                                         *
 * Behaviour summary:
 *  - Samples raw magnetometer XYZ values and tracks min/max for each axis
 *  - Displays live samples and computed ranges on the console
 *  - Stops when the user presses ESC
 *  - Offers to save computed offsets (center of min/max) to EEPROM         *
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

/* No public includes required; implementation includes project headers
   (Configuration, Globals, sensor drivers, etc.) */

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* No public constants required for callers */

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/* No public types */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuCalibrateCompass
 *
 * Entry point for the magnetometer calibration menu.
 *
 * Behaviour:
 *  - Verifies that a navigation compass is available (gConfiguration.ram.navCompassAvailable)
 *  - Samples magnetometer values at regular intervals and updates per-axis min/max
 *  - Prints live samples and computed ranges to the console
 *  - Exits when the user presses ESC and prompts to save computed offsets
 *
 * On user confirmation the computed offsets are stored in:
 *   gConfiguration.eeprom.xMagOffset
 *   gConfiguration.eeprom.yMagOffset
 *   gConfiguration.eeprom.zMagOffset
 *
 * The function performs IO to the console and may block until user input.
 */
void MenuCalibrateCompass(void);

