/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Console menu system implementation                            *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This module implements a simple console-based menu manager that:        *
 * - Displays numbered menu entries                                        *
 * - Processes numeric input to select entries                            *
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
 ***************************************************************************/

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "MenuManager.h"
#include "Globals.h"
#include "BoardConfig.h"

#include <Arduino.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* No file-local constants required */

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/* No local types required */

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/* No local prototypes required */

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/* No file-local globals required */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * Default constructor
 * 
 * Initializes menu pointer and length to safe values
 */
MenuManager::MenuManager()
{
    menu       = nullptr;
    menuLength = 0;
}

/**
 * Default destructor
 */
MenuManager::~MenuManager()
{
}

/**
 * Configure the menu structure to be managed
 * 
 * @param menuDesc Pointer to array of MenuEntry_t, nullptr terminated
 */
void MenuManager::SetMenuDescription(const MenuEntry_t *menuDesc)
{
    if (menuDesc != nullptr)
    {
        menu = menuDesc;
        while (menu[menuLength].description != nullptr)
        {
            menuLength++;
        }
    }
}

/**
 * Process a character received from the console
 * 
 * Handles:
 * - Numeric input (1-9) to select menu entries
 * - 0 to reprint the menu
 * - Executes associated callback if entry is valid
 * 
 * @param c Character to process
 */
void MenuManager::PushChar(char c)
{
    if ((c > 0x30) && (c <= 0x39))
    {
        int entry = c - 0x30;
        if (entry < menuLength)
        {
            if (menu[entry].entryCallback != nullptr)
            {
                CONSOLE.println(entry);
                CONSOLE.println("");
                menu[entry].entryCallback();
                PrintPrompt();
            }
        }
    }
    else if (c == 0x30)
    {
        CONSOLE.println("0");
        PrintMenu();
        PrintPrompt();
    }
}

/**
 * Print the complete menu to the console
 * 
 * Format:
 * *** Title ***
 * 0 - Print this menu
 * 1 - First entry
 * 2 - Second entry
 * etc.
 */
void MenuManager::PrintMenu()
{
    if ((menu == nullptr) || (menuLength < 2))
    {
        return;
    }

    CONSOLE.println("");
    CONSOLE.print("*** ");
    CONSOLE.print(menu[0].description);
    CONSOLE.println(" ***");
    CONSOLE.println("");
    CONSOLE.println("0 - Print this menu");
    for (int i = 1; i < menuLength; i++)
    {
        CONSOLE.print(i);
        CONSOLE.print(" - ");
        CONSOLE.println(menu[i].description);
    }
}

/**
 * Activate a specific menu entry by index
 * 
 * @param entry Index of the menu entry to activate
 */
void MenuManager::ActivateMenu(uint32_t entry)
{
    if (entry < (uint32_t)menuLength)
    {
        if (menu[entry].entryCallback != nullptr)
        {
            menu[entry].entryCallback();
        }
        else
        {
            PrintPrompt();
        }
    }
}

/**
 * Print the menu prompt
 * 
 * Displays "Choice : " and waits for user input
 */
void MenuManager::PrintPrompt()
{
    CONSOLE.println("");
    CONSOLE.print("Choice : ");
}
