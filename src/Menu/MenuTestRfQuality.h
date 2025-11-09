/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Test RF link quality between Micronet devices                 *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the RF quality tester.         *
 * The implementation provides an interactive tool that:                   *
 * - Sends periodic ping messages to network devices                       *
 * - Measures signal strength for responses                               *
 * - Reports link quality metrics                                         *
 * - Maps device types and roles                                          *
 *                                                                         *
 * Note: This diagnostic tool uses the async slot which may interfere     *
 * with normal network operation. Not recommended during navigation.       *
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
 * MenuTestRfQuality
 *
 * Entry point for the RF link quality tester. When called, this function:
 *  - Monitors master requests to sync with network timing
 *  - Sends ping messages to all devices in the async slot
 *  - Measures signal strength (RSSI) of responses
 *  - Reports for each device:
 *    * Device ID and type
 *    * Link quality to this converter (LNK metric)
 *    * Link quality to network master (NET metric) 
 *    * Master device indicator [M]
 *
 * Warning: This test uses the async slot which may interfere with normal
 * network operation. Not recommended during navigation.
 *
 * The function performs blocking console I/O and returns when the user
 * presses ESC.
 */
void MenuTestRfQuality(void);

