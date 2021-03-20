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

#include "MenuManager.h"

#include "Arduino.h"

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
	console = &Serial;
}

MenuManager::~MenuManager()
{

}

void MenuManager::SetConsole(Stream *console)
{
	this->console = console;
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
		if ((entry > 0) && (entry < menuLength))
		{
			if (menu[entry].entryCallback != nullptr)
			{
				console->println(entry);
				console->println("");
				menu[entry].entryCallback();
				PrintPrompt();
			}
		}
	} else if (c == 0x30) {
		console->println("0");
		PrintMenu();
	}
}

void MenuManager::PrintMenu()
{
	if ((menu == nullptr) || (menuLength < 2))
	{
		return;
	}

	console->println("");
	console->print("*** ");
	console->print(menu[0].description);
	console->println(" ***");
	console->println("");
	console->println("0 - Print this menu");
	for (int i = 1; i < menuLength; i++)
	{
		console->print(i);
		console->print(" - ");
		console->println(menu[i].description);
	}
	PrintPrompt();
}

void MenuManager::PrintPrompt()
{
	console->println("");
	console->print("Choice : ");
}
