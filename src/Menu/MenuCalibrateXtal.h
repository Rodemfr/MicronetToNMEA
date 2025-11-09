/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  RF frequency calibration menu declaration                     *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the RF oscillator calibration  *
 * routine (MenuCalibrateXtal). The implementation performs an interactive *
 * frequency sweep of the CC1101-based receiver to determine the working   *
 * frequency range for the Micronet master transmitter and optionally      *
 * saves the computed frequency offset into persistent configuration.      *
 *                                                                         *
 * Behaviour summary:
 *  - Guides the user to place devices close together and remain stationary
 *  - Temporarily adjusts RF driver settings for improved sensitivity
 *  - Sweeps frequency over a defined range and counts received messages
 *  - Computes first/last working frequency and derives a center frequency
 *  - Offers to persist the computed RF calibration to EEPROM               *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021-2025 Ronan Demoment                                *
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
 ***************************************************************************/

#pragma once

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

/* Implementation includes project headers (BoardConfig, Globals, RF driver,
   Micronet codec, message FIFO). No public headers are required by callers. */

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
 * MenuCalibrateXtal
 *
 * Entry point for the RF frequency calibration menu.
 *
 * Responsibilities:
 *  - Prompt the user and wait for confirmation to start tuning
 *  - Temporarily disable frequency tracking and narrow receiver bandwidth
 *  - Sweep the receiver frequency across a predefined range and count
 *    Master Request messages to determine the working frequency window
 *  - Compute the center frequency and range, display results and offer
 *    to save the calibration into persistent configuration (EEPROM)
 *  - Restore RF driver settings on exit
 *
 * The function performs blocking console IO and returns when tuning completes
 * or when the user cancels (ESC). On confirmation the computed offset is
 * stored in gConfiguration.eeprom.rfFrequencyOffset_MHz and persisted.
 */
void MenuCalibrateXtal(void);

