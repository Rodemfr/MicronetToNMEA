/*
 * MenuManager.cpp
 *
 *  Created on: 10 mars 2021
 *      Author: Ronan
 */

#include "MenuManager.h"

#include "Arduino.h"

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
		if ((entry > 0) && (entry < menuLength))
		{
			if (menu[entry].entryCallback != nullptr)
			{
				Serial.println(entry);
				Serial.println("");
				menu[entry].entryCallback();
				PrintPrompt();
			}
		}
	} else if (c == 0x30) {
		Serial.println("0");
		PrintMenu();
	}
}

void MenuManager::PrintMenu()
{
	if ((menu == nullptr) || (menuLength < 2))
	{
		return;
	}

	Serial.println("");
	Serial.print("*** ");
	Serial.print(menu[0].description);
	Serial.println(" ***");
	Serial.println("");
	Serial.println("0 - Print this menu");
	for (int i = 1; i < menuLength; i++)
	{
		Serial.print(i);
		Serial.print(" - ");
		Serial.println(menu[i].description);
	}
	Serial.println("");
	PrintPrompt();
}

void MenuManager::PrintPrompt()
{
	Serial.print("Choice : ");
}
