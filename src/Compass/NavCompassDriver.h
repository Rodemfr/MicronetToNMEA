/*
 * NavCompassDriver.h
 *
 *  Created on: 11 sept. 2021
 *      Author: Ronan
 */

#pragma once

#include "I2cMaster.h"

#include <string>

struct Vec3D
{
    float x, y, z;
};

class NavCompassDriver
{
  public:
    virtual ~NavCompassDriver()                      = 0;
    virtual bool        Init(I2cMaster *i2cMaster)   = 0;
    virtual std::string GetDeviceName()              = 0;
    virtual void        GetMagneticField(Vec3D *mag) = 0;
    virtual void        GetAcceleration(Vec3D *acc)  = 0;
};
