/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Decode data from Micronet devices send it on an NMEA network  *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021 by Ronan Demoment                                  *
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
 ***************************************************************************
 */

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "BoardConfig.h"
#include "MenuManager.h"

#include <Arduino.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

MenuManager::MenuManager()
{
	menuLength = 0;
	menu = nullptr;
}

MenuManager::~MenuManager()
{

}

void MenuManager::SetMenu(MenuEntry_t *menu)
{
	this->menu = menu;
	menuLength = 0;
	while (menu[menuLength].description != nullptr)
	{
		menuLength++;
	}
}

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
	} else if (c == 0x30) {
		CONSOLE.println("0");
		PrintMenu();
	}
}

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
	PrintPrompt();
}

void MenuManager::PrintPrompt()
{
	CONSOLE.println("");
	CONSOLE.print("Choice : ");
}
