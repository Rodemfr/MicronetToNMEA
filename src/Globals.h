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

#ifndef GNSSDECODER_H_
#define GNSSDECODER_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "Configuration.h"
#include "DataBridge.h"
#include "M8NDriver.h"
#include "MenuManager.h"
#include "MicronetCodec.h"
#include "MicronetMessageFifo.h"
#include "MicronetSlaveDevice.h"
#include "NavCompass.h"
#include "NavigationData.h"
#include "RfDriver.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

extern RfDriver            gRfReceiver;
extern MenuManager         gMenuManager;
extern MicronetMessageFifo gRxMessageFifo;
extern Configuration       gConfiguration;
extern NavCompass          gNavCompass;
extern M8NDriver           gM8nDriver;

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* GNSSDECODER_H_ */
