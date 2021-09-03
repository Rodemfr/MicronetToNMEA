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

MicronetMessageFifo::MicronetMessageFifo()
{
	// Reset packet store
	writeIndex = 0;
	readIndex = 0;
	nbMessages = 0;
}

MicronetMessageFifo::~MicronetMessageFifo()
{
}

bool MicronetMessageFifo::Push(MicronetMessage_t &message)
{
	// Disable interrupts to avoid race conditions
	noInterrupts();
	// Check if there is space in store. If not, the message is just dropped/ignored.
	if (nbMessages < MESSAGE_STORE_SIZE)
	{
		// Yes : copy message to the store and update store's status
		store[writeIndex].len = message.len;
		store[writeIndex].rssi = message.rssi;
		store[writeIndex].startTime_us = message.startTime_us;
		store[writeIndex].endTime_us = message.endTime_us;
		memcpy(store[writeIndex].data, message.data, message.len);
		writeIndex++;
		nbMessages++;
		if (writeIndex >= MESSAGE_STORE_SIZE)
		{
			writeIndex = 0;
		}
	}
	else
	{
		interrupts();
		return false;
	}
	interrupts();

	return true;
}

bool MicronetMessageFifo::Pop(MicronetMessage_t *message)
{
	// Disable interrupts to avoid race conditions
	noInterrupts();

	// Are there messages in the store ?
	if (nbMessages > 0)
	{
		// Yes : Copy message
		memcpy(message, &(store[readIndex]), sizeof(MicronetMessage_t));
		// Remove message from the store
		readIndex++;
		if (readIndex >= MESSAGE_STORE_SIZE)
		{
			readIndex = 0;
		}
		nbMessages--;
	}
	else
	{
		interrupts();
		return false;
	}
	interrupts();

	return true;
}

MicronetMessage_t *MicronetMessageFifo::Peek()
{
	MicronetMessage_t *pMessage = nullptr;

	// Disable interrupts to avoid race conditions
	noInterrupts();

	// Are there messages in the store ?
	if (nbMessages > 0)
	{
		pMessage = &(store[readIndex]);
	}

	interrupts();

	return pMessage;
}

void MicronetMessageFifo::DeleteMessage()
{
	noInterrupts();

	// Are there messages in the store ?
	if (nbMessages > 0)
	{
		// Yes : delete the next one
		nbMessages--;
		readIndex++;
		if (readIndex >= MESSAGE_STORE_SIZE)
		{
			readIndex = 0;
		}
	}

	interrupts();
}

void MicronetMessageFifo::ResetFifo()
{
	noInterrupts();
	readIndex = writeIndex;
	nbMessages = 0;
	interrupts();
}
