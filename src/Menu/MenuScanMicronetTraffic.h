/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Monitor and analyze Micronet radio traffic                    *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the Micronet traffic scanner.  *
 * The implementation provides real-time monitoring and analysis of:        *
 * - Raw message contents and timing                                       *
 * - Signal strength measurements                                          *
 * - Network topology discovery                                           *
 * - Protocol timing analysis                                             *
 *                                                                         *
 * This tool is intended for:                                             *
 * - Network debugging and troubleshooting                                *
 * - Understanding network behavior                                        *
 * - RF reception quality verification                                     *
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

/* No public includes required; implementation includes project headers */

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
 * MenuScanMicronetTraffic
 *
 * Entry point for the Micronet traffic scanner. When called, this function:
 *  - Initializes the RF receiver and message FIFO
 *  - Enters a monitoring loop that:
 *    * Displays received messages with timing and signal strength
 *    * Updates network topology map on master requests
 *    * Shows detailed message contents and protocol analysis
 *  - Exits when user presses ESC
 *
 * The function performs blocking console I/O but yields regularly to allow
 * background tasks to run. It is intended for interactive debugging and
 * analysis sessions.
 */
void MenuScanMicronetTraffic(void);

