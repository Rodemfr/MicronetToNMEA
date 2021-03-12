/*
 * MenuManager.cpp
 *
 *  Created on: 10 mars 2021
 *      Author: Ronan
 */

#include "MenuManager.h"

#include "Arduino.h"

MenuEntry_t serialMenu[] = {
		{"Set serial speed", nullptr, nullptr},
		{"Set serial mode", nullptr, nullptr},
		{nullptr, nullptr}
};

MenuEntry_t deviceMenu[] = {
		{"Print list of paired devices", nullptr, nullptr},
		{"Detect devices in range", nullptr, nullptr},
		{"Add a sensor to the list", nullptr, nullptr},
		{"Remove a sensor from the list", nullptr, nullptr},
		{"Save sensor list to EEPROM", nullptr, nullptr},
		{nullptr, nullptr}
};

MenuEntry_t calibrationMenu[] = {
		{"Calibrate wind transducer", nullptr, nullptr},
		{"Calibrate depth transducer", nullptr, nullptr},
		{"Calibrate speed transducer", nullptr, nullptr},
		{nullptr, nullptr}
};

MenuEntry_t mainMenu[] = {
		{"Serial menu", nullptr, serialMenu},
		{"Device menu", nullptr, deviceMenu},
		{"Calibration menu", nullptr, calibrationMenu},
		{"Start NMEA conversion", nullptr, nullptr},
		{nullptr, nullptr, nullptr}
};

MenuManager::MenuManager()
{
	currentLevel = 0;
}

MenuManager::~MenuManager()
{

}

void MenuManager::PushChar(char c)
{

}

void MenuManager::PrintCurrentMenu()
{

}
