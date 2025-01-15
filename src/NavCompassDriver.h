/*
 * NavCompassDriver.h
 *
 *  Created on: 11 sept. 2021
 *      Author: Ronan
 */

#ifndef NAVCOMPASSDRIVER_H_
#define NAVCOMPASSDRIVER_H_

#include <string>

using string = std::string;

struct Vec3D
{
    float x, y, z;
};

class NavCompassDriver
{
  public:
    virtual ~NavCompassDriver()                 = 0;
    virtual bool   Init()                       = 0;
    virtual string GetDeviceName()              = 0;
    virtual void   GetMagneticField(Vec3D *mag) = 0;
    virtual void   GetAcceleration(Vec3D *acc)  = 0;
};

#endif /* NAVCOMPASSDRIVER_H_ */
