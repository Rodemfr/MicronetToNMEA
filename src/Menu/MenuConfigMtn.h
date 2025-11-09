/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Main configuration menu handler for MicronetToNMEA            *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the main configuration menu.   *
 * The implementation provides a console-based menu system that allows     *
 * users to configure all device settings including:                       *
 * - Data source links (NMEA, Micronet, internal sensors)                 *
 * - SOG/COG filtering parameters                                         *
 * - Device orientation and calibration settings                          *
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
 * MenuConfigMtn
 *
 * Entry point for the main configuration menu system. When called, this 
 * function displays and manages a console-based menu that allows users to:
 *  - Configure data source links between NMEA, Micronet and internal sensors
 *  - Adjust SOG/COG filtering parameters
 *  - Set device orientation and calibration values
 *  - Save settings to persistent storage
 *
 * The function performs blocking console I/O and returns when the user
 * selects "Return to main menu" or presses ESC.
 */
void MenuConfigMtn(void);

