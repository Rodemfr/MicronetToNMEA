/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Multi client SPP driver                                       *
 * Author:   Ronan Demoment                                                *
 *                                                                         *
 ***************************************************************************
 *   Copyright (C) 2025 by Ronan Demoment                                  *
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

#include <Stream.h>
#include <esp_spp_api.h>
#include <queue>
#include <string>
#include <vector>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define SPP_RX_BUFFER_SIZE 2048 // Size of the RX buffer for each SPP client

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

/**
 * @class BtMultiSPP
 * @brief Bluetooth SPP server supporting multiple simultaneous clients.
 *
 * This class implements a Bluetooth Serial Port Profile (SPP) server using the ESP-IDF API.
 * Unlike the standard Arduino `BluetoothSerial` class, which supports only one client at a time,
 * `BtMultiSPP` allows multiple concurrent SPP client connections, up to the maximum value by ESP-IDF API.
 *
 * Each client has its own circular buffer (2 KB), which stores incoming data.
 * If the buffer overflows, old data is overwritten (wrap-around behavior).
 *
 * Key features:
 * - Accepts up to 2 simultaneous Bluetooth SPP connections
 * - Circular buffer per client for received data
 * - Selectable active client for read/write operations
 * - Basic congestion handling with logging
 *
 * Typical usage:
 * - Call `begin()` to start the Bluetooth SPP server
 * - Use `write()` to send data to the currently active client
 * - Use `read()` and `available()` to receive data from the active client
 * - Switch between clients using `setActiveClient(index)`
 */
class BtMultiSPP : public Stream
{
  public:
    BtMultiSPP();
    ~BtMultiSPP();

    bool begin(const std::string &deviceName = "Multi SPP");
    void end();

    size_t write(uint8_t data);
    size_t write(const uint8_t *data, size_t len);
    int    read();
    int    peek();
    int    available();

    void setActiveClient(int clientIndex);
    int  getClientCount();

  private:
    static void sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
    void        handleDataReceived(uint32_t handle, uint8_t *data, size_t len);
    void        handleClientConnect(uint32_t handle);
    void        handleClientDisconnect(uint32_t handle);

    struct Client
    {
        uint32_t handle;
        uint8_t  buffer[SPP_RX_BUFFER_SIZE];
        size_t   head = 0;
        size_t   tail = 0;

        void push(uint8_t byte);
        void push(uint8_t *data, uint32_t len);
        int  available() const;
        int  pop();
        int  peek();
    };

    static BtMultiSPP *instance;

    std::vector<Client> clients;
    int                 activeClientIndex;
    std::string         deviceName;
    bool                initialized;
};
