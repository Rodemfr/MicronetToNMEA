/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Configure compass/magnetometer mounting orientation            *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the compass orientation        *
 * configuration menu. The implementation provides a console-based menu    *
 * that allows users to specify how the compass/magnetometer is mounted    *
 * by selecting which axes represent:                                      *
 * - The heading/bow direction (forward direction of the boat)            *
 * - The down direction (vertical alignment)                              *
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
 * MenuConfigOrientation
 *
 * Entry point for the compass orientation configuration menu. When called, 
 * this function displays and manages a console-based menu that allows 
 * users to:
 *  - Select which axis represents the heading/bow direction
 *  - Select which axis represents the down direction
 *  - Save settings to persistent storage
 *
 * The function ensures selected axes remain different while cycling through
 * options. It performs blocking console I/O and returns when the user
 * selects to save and exit or cancels the operation.
 */
void MenuConfigOrientation(void);

