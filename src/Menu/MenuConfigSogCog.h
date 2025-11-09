/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Configure SOG/COG filtering parameters                         *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the SOG/COG configuration     *
 * menu. The implementation provides a console-based menu that allows      *
 * users to configure:                                                     *
 * - SOG (Speed Over Ground) and COG (Course Over Ground) filtering       *
 * - Filter time constant/strength                                         *
 * - Water speed emulation from SOG data                                  *
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
 * MenuConfigSogCog
 *
 * Entry point for the SOG/COG configuration menu. When called, this 
 * function displays and manages a console-based menu that allows users to:
 *  - Enable/disable SOG and COG filtering
 *  - Adjust filter strength (time constant)
 *  - Enable/disable water speed emulation from SOG
 *  - Save settings to persistent storage
 *
 * The function performs blocking console I/O and returns when the user
 * selects to save and exit or cancels the operation.
 */
void MenuConfigSogCog(void);

