/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Scan for nearby Micronet networks and allow the user to       *
 *           attach the converter to the closest one.                      *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the "Attach to network" menu.  *
 * The implementation scans radio traffic for Micronet packets, collects   *
 * distinct network identifiers (NID) ordered by RSSI, displays results to *
 * the user and optionally persists the selected NID into EEPROM.         *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021 by Ronan Demoment                                  *
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

/* No public includes required; implementation includes project headers.   */

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* No public constants required */

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/* No public types required */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuAttachNetwork
 *
 * Scans the radio for Micronet networks during a short time window,
 * collects unique network identifiers (NID) ordered by signal strength (RSSI),
 * presents the list to the user and offers to attach the converter to the
 * closest/strongest network.
 *
 * Behaviour:
 *  - Blocking, console-driven operation suitable for interactive use
 *  - Verifies message CRC before considering a network valid
 *  - Updates gConfiguration.eeprom.networkId and persists when user confirms
 *
 * The function returns after the scan and optional user confirmation.
 */
void MenuAttachNetwork(void);

