/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Configure logical data source links (GNSS, wind, depth, speed *
 *           and compass) for the conversion pipeline                      *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the interactive menu that lets *
 * the user select which physical or logical input provides navigation and *
 * sensor data used by the converter. Options typically include a connected *
 * plotter, a NMEA0183 input, Micronet itself or the onboard compass.      *
 *                                                                         *
 * Behaviour summary:
 *  - Presents choices for each logical data type (GNSS, wind, depth, speed *
 *    and compass) to the user via the console
 *  - Validates and maps stored configuration values to available choices
 *  - Persists user selections into EEPROM when requested                   *
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

/* No public includes required; implementation includes project headers
   (Configuration, Globals, Micronet, etc.) */

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* No public constants required by callers */

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/* No public types */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuConfigLink
 *
 * Entry point invoked by the menu system to configure source links for:
 *  - GNSS (position/time)
 *  - Wind (direction/speed)
 *  - Depth
 *  - Speed (through water / over ground)
 *  - Compass (magnetic heading)
 *
 * Behaviour:
 *  - Reads current selections from persistent configuration and maps them
 *    to available choices
 *  - Presents a simple console menu allowing the user to cycle options
 *  - Saves updated selections to EEPROM when the user chooses to persist
 *
 * The function performs console I/O and may block until the user exits the
 * menu or confirms saving. It does not return until the menu interaction
 * completes.
 */
void MenuConfigLink(void);

