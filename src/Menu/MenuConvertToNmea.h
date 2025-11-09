/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Convert Micronet data and forward it onto an NMEA0183 network *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 * This header declares the entry point for the "Convert to NMEA" menu.    *
 * The corresponding implementation initializes the conversion pipeline,   *
 * loads/saves calibration, configures the Micronet slave device and runs  *
 * the main loop which forwards Micronet messages as NMEA sentences while  *
 * handling incoming NMEA input (from a plotter or NMEA IN) and user input.*
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2021 by Ronan Demoment                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

/* No public headers required by callers; implementation includes project
   headers (Configuration, Globals, Micronet, etc.) */

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

/* No public constants */

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/* No public types */

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/**
 * MenuConvertToNmea
 *
 * Entry point invoked by the menu system when the user selects
 * "Start NMEA conversion".
 *
 * Responsibilities:
 *  - Ensure converter is attached to a Micronet network
 *  - Initialize Micronet codec, DataBridge and Micronet slave device
 *  - Load calibration from persistent storage and apply configuration
 *  - Enter main processing loop:
 *      * Process incoming Micronet messages and forward converted NMEA
 *      * Handle incoming NMEA input and pass it to the DataBridge
 *      * Periodically sample onboard sensors (compass, GNSS) if present
 *      * Save calibration updates when required
 *
 * The function returns when the user exits the conversion mode (e.g. presses ESC).
 */
void MenuConvertToNmea(void);

