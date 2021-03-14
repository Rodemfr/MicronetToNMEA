/*
 * MenuManager.h
 *
 *  Created on: 10 mars 2021
 *      Author: Ronan
 */

#ifndef MENUMANAGER_H_
#define MENUMANAGER_H_

typedef struct MenuEntry_t
{
	const char *description;
	void (*entryCallback)(void);
} MenuEntry_t;

#define MAX_MENU_DEPTH 4

class MenuManager {
public:
	MenuManager();
	virtual ~MenuManager();

	void SetMenu(MenuEntry_t *menu);
	void PushChar(char c);
	void PrintMenu();

private:
	MenuEntry_t *menu;
	int menuLength;

	void PrintPrompt();
};

#endif /* MENUMANAGER_H_ */
