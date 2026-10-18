/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Driver for LSM303DLH                                          *
 * Author:   Ronan Demoment, Dietmar Warning                               *
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

#include "I2cMaster.h"
#include "NavCompassDriver.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class LSM303DLHDriver : public NavCompassDriver
{
  public:
    LSM303DLHDriver();
    virtual ~LSM303DLHDriver();

    virtual bool        Init(I2cMaster *i2cMaster) override;
    virtual std::string GetDeviceName() override;
    virtual void        GetMagneticField(Vec3D *mag) override;
    virtual void        GetAcceleration(Vec3D *acc) override;

  private:
    I2cMaster            *i2cMaster;
    I2cMasterDeviceHandle magDeviceHandle;
    I2cMasterDeviceHandle accDeviceHandle;
    float                 LsbPerGaussXY;
    float                 LsbPerGaussZ;
    float                 GPerLsb;

    void UnregisterSlaves();
};
