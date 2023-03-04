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

#include <Arduino.h>

#include "BoardConfig.h"
#include "Configuration.h"
#include "Globals.h"
#include "Micronet.h"
#include "MicronetCodec.h"
#include "MicronetMessageFifo.h"

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

void MenuAttachNetwork()
{
    char     input[16], c;
    uint32_t charIndex = 0;

    CONSOLE.print("Enter Network ID to attach to : ");

    do
    {
        if (CONSOLE.available())
        {
            c = CONSOLE.read();
            if (c == 0x0d)
            {
                CONSOLE.println("");
                break;
            }
            else if ((c == 0x08) && (charIndex > 0))
            {
                charIndex--;
                CONSOLE.print(c);
                CONSOLE.print(" ");
                CONSOLE.print(c);
            }
            else if (charIndex < sizeof(input))
            {
                input[charIndex++] = c;
                CONSOLE.print(c);
            }
        };
    } while (1);

    bool     invalidInput = false;
    uint32_t newNetworkId = 0;

    if (charIndex == 0)
    {
        invalidInput = true;
    }

    for (uint32_t i = 0; i < charIndex; i++)
    {
        c = input[i];
        if ((c >= '0') && (c <= '9'))
        {
            c -= '0';
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            c = c - 'a' + 10;
        }
        else if ((c >= 'A') && (c <= 'F'))
        {
            c = c - 'A' + 10;
        }
        else
        {
            invalidInput = true;
            break;
        }

        newNetworkId = (newNetworkId << 4) | c;
    }

    if (invalidInput)
    {
        CONSOLE.println("Invalid Network ID entered, ignoring input.");
    }
    else
    {
        gConfiguration.networkId = newNetworkId;
        CONSOLE.print("Now attached to NetworkID ");
        CONSOLE.println(newNetworkId, HEX);
        gConfiguration.SaveToEeprom();
    }
}
