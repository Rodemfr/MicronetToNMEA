/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Console-based menu system manager                             *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements a simple menu manager that:                      *
 * - Displays numbered menu entries on the console                         *
 * - Processes numeric input to select entries                             *
 * - Executes associated callback functions                               *
 * - Maintains menu state and handles reprinting                          *
 *                                                                         *
 * The implementation is designed to be:                                   *
 * - Memory efficient (uses const char* for strings)                       *
 * - Non-blocking (processes one character at a time)                      *
 * - Reusable across different menu hierarchies                           *
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

#include <Arduino.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/**
 * Maximum depth of menu nesting supported
 */
#define MAX_MENU_DEPTH 4

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/**
 * Menu entry structure
 *
 * @param description Text to display for this menu entry
 * @param entryCallback Function to call when entry is selected
 *                     nullptr for title or end of menu marker
 */
typedef struct MenuEntry_t
{
    const char *description;
    void (*entryCallback)(void);
} MenuEntry_t;

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

/**
 * MenuManager
 *
 * This class implements a simple console-based menu system.
 * It displays numbered entries, processes numeric input and
 * executes associated callbacks.
 */
class MenuManager
{
  public:
    MenuManager();
    virtual ~MenuManager();

    void SetMenuDescription(const MenuEntry_t *menuDesc);
    void PushChar(char c);
    void PrintMenu();
    void PrintPrompt();
    void ActivateMenu(uint32_t entry);

  private:
    const MenuEntry_t *menu;
    int                menuLength;
};
