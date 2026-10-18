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

#pragma once

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "BtMultiSPP.h"
#include "Configuration.h"
#include "EspSerial.h"
#include "M8NDriver.h"
#include "RfDriver.h"
#include "NavCompass.h"
#include "MenuManager.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

extern Configuration       gConfiguration;
extern BtMultiSPP          gBtMultiSPP;
extern EspSerial           gSerial1;
extern EspSerial           gSerial2;
extern M8NDriver           gM8nDriver;
extern RfDriver            gRfReceiver;
extern MicronetMessageFifo gRxMessageFifo;
extern NavCompass          gNavCompass;
extern MenuManager         gMenuManager;


/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/
