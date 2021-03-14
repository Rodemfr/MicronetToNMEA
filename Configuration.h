/*
 * Configuration.h
 *
 *  Created on: 14 mars 2021
 *      Author: Ronan
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <stdint.h>

class Configuration
{
public:
	Configuration();
	virtual ~Configuration();

	uint32_t attachedNetworkId;
	uint32_t serialSpeed;
};

#endif /* CONFIGURATION_H_ */
