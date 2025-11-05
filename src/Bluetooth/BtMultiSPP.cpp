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

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "BtMultiSPP.h"
#include <esp_bt.h>
#include <esp_bt_device.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_log.h>
#include <string.h>

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
/*                             Attributes                                  */
/***************************************************************************/

BtMultiSPP *BtMultiSPP::instance = nullptr;

/***************************************************************************/
/*                               Methods                                   */
/***************************************************************************/

/**
 * @brief Constructor.
 */
BtMultiSPP::BtMultiSPP() : activeClientIndex(0), initialized(false)
{
    instance = this; // Save reference for static callback
}

/**
 * @brief Destructor.
 */
BtMultiSPP::~BtMultiSPP()
{
    end();
}

/**
 * @brief Initializes the Bluetooth stack and starts the SPP server.
 *
 * @param name The Bluetooth device name to be advertised.
 * @return true on success, false on failure.
 */
bool BtMultiSPP::begin(const std::string &name)
{
    if (initialized)
        return true;

    deviceName = name;
    esp_err_t ret;

    // Release BLE memory if previously used
    ret = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    if (ret)
        return false;

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret                               = esp_bt_controller_init(&bt_cfg);
    if (ret)
        return false;

    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret)
        return false;

    ret = esp_bluedroid_init();
    if (ret)
        return false;

    ret = esp_bluedroid_enable();
    if (ret)
        return false;

    // Register SPP callback and initialize SPP
    esp_spp_register_callback(BtMultiSPP::sppCallback);
    esp_spp_init(ESP_SPP_MODE_CB);

    esp_bt_dev_set_device_name(deviceName.c_str());
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    initialized = true;
    return true;
}

/**
 * @brief Stops the Bluetooth service and releases resources.
 */
void BtMultiSPP::end()
{
    if (!initialized)
        return;

    esp_spp_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    clients.clear();
    initialized = false;
}

/**
 * @brief Sets the active client for read/write operations.
 *
 * @param index The index of the client (0 or 1).
 */
void BtMultiSPP::setActiveClient(int index)
{
    if (index >= 0 && index < clients.size())
    {
        activeClientIndex = index;
    }
}

/**
 * @brief Returns the number of currently connected clients.
 *
 * @return Number of clients (0, 1, or 2).
 */
int BtMultiSPP::getClientCount()
{
    return clients.size();
}

/**
 * @brief Sends a single byte to the active client.
 *
 * @param data Byte to send.
 * @return Number of bytes written (1 if successful).
 */
size_t BtMultiSPP::write(uint8_t data)
{
    return write(&data, 1);
}

/**
 * @brief Sends a block of data to the active client.
 *
 * @param data Pointer to the data buffer.
 * @param len Length of the buffer.
 * @return Number of bytes written.
 */
size_t BtMultiSPP::write(const uint8_t *data, size_t len)
{
    if (activeClientIndex >= clients.size())
    {
        return 0;
    }
    esp_spp_write(clients[activeClientIndex].handle, len, const_cast<uint8_t *>(data));
    return len;
}

/**
 * @brief Reads one byte from the active client's input buffer.
 *
 * @return Byte read, or -1 if buffer is empty.
 */
int BtMultiSPP::read()
{
    if (activeClientIndex >= clients.size())
        return -1;
    return clients[activeClientIndex].pop();
}

/**
 * @brief Reads one byte from the active client's input buffer, without removing it.
 *
 * @return Byte read, or -1 if buffer is empty.
 */
int BtMultiSPP::peek()
{
    if (activeClientIndex >= clients.size())
        return -1;
    return clients[activeClientIndex].peek();
}

/**
 * @brief Returns the number of bytes available for reading from the active client.
 *
 * @return Number of bytes in buffer.
 */
int BtMultiSPP::available()
{
    if (activeClientIndex >= clients.size())
        return 0;
    return clients[activeClientIndex].available();
}

/**
 * @brief Static callback for handling SPP events from the ESP-IDF stack.
 *
 * @param event SPP event type.
 * @param param Event parameters.
 */
void BtMultiSPP::sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    if (!instance)
        return;

    switch (event)
    {
    case ESP_SPP_INIT_EVT:
        // Start the SPP server
        esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "SPP_MultiSerial");
        break;

    case ESP_SPP_DATA_IND_EVT:
        // Data received from client
        instance->handleDataReceived(param->data_ind.handle, param->data_ind.data, param->data_ind.len);
        break;

    case ESP_SPP_SRV_OPEN_EVT:
        // New client connected
        instance->handleClientConnect(param->srv_open.handle);
        break;

    case ESP_SPP_CLOSE_EVT:
        // Client disconnected
        instance->handleClientDisconnect(param->close.handle);
        break;

    default:
        break;
    }
}

/**
 * @brief Handles incoming data from a connected client.
 *
 * @param handle Connection handle of the client.
 * @param data Pointer to the data received.
 * @param len Length of the data.
 */
void BtMultiSPP::handleDataReceived(uint32_t handle, uint8_t *data, size_t len)
{
    for (auto &client : clients)
    {
        if (client.handle == handle)
        {
            client.push(data, len);
            break;
        }
    }
}

/**
 * @brief Handles a new client connection.
 *
 * @param handle Connection handle of the new client.
 */
void BtMultiSPP::handleClientConnect(uint32_t handle)
{
    Client c;
    c.handle = handle;
    clients.push_back(c);
}

/**
 * @brief Handles disconnection of a client.
 *
 * @param handle Connection handle of the client that disconnected.
 */
void BtMultiSPP::handleClientDisconnect(uint32_t handle)
{
    for (auto it = clients.begin(); it != clients.end(); ++it)
    {
        if (it->handle == handle)
        {
            clients.erase(it);
            if (activeClientIndex >= clients.size())
                activeClientIndex = 0;
            break;
        }
    }
}

/**
 * @brief Pushes a byte into the circular buffer.
 *
 * If the buffer is full, the oldest byte is overwritten (wrap-around behavior).
 *
 * @param byte The byte to be added to the buffer.
 */
void BtMultiSPP::Client::push(uint8_t byte)
{
    buffer[head] = byte;
    head         = (head + 1) % SPP_RX_BUFFER_SIZE;
    if (head == tail)
    {
        // Buffer full: overwrite the oldest data
        tail = (tail + 1) % SPP_RX_BUFFER_SIZE;
    }
}

/**
 * @brief Returns the number of bytes currently stored in the buffer.
 *
 * @return The number of bytes available to read.
 */
int BtMultiSPP::Client::available() const
{
    if (head >= tail)
    {
        return head - tail;
    }
    return SPP_RX_BUFFER_SIZE - (tail - head);
}

/**
 * @brief Pushes a block of bytes into the client's circular buffer.
 *
 * This method writes up to `len` bytes from the input `data` array into the circular buffer.
 * If the amount of data exceeds the available space in the buffer, the oldest data is overwritten
 * to make room (wrap-around behavior). The write is performed using one or two memcpy() operations
 * for efficiency, depending on whether the write crosses the end of the buffer.
 *
 * @param data Pointer to the array of bytes to be pushed into the buffer.
 * @param len Number of bytes to write from the array.
 *
 * @note If `len` is greater than or equal to the buffer size, only the last `BUFFER_SIZE` bytes
 *       of the input will be kept.
 */
void BtMultiSPP::Client::push(uint8_t *data, uint32_t len)
{
    // If the incoming data length is >= buffer size, keep only the last part
    if (len >= SPP_RX_BUFFER_SIZE)
    {
        data += (len - SPP_RX_BUFFER_SIZE);
        len = SPP_RX_BUFFER_SIZE;
    }

    uint32_t spaceToEnd = SPP_RX_BUFFER_SIZE - head;

    if (len <= spaceToEnd)
    {
        // No wrap-around: single memcpy
        memcpy(&buffer[head], data, len);
        head = (head + len) % SPP_RX_BUFFER_SIZE;
    }
    else
    {
        // Wrap-around: two memcpy
        memcpy(&buffer[head], data, spaceToEnd);
        memcpy(&buffer[0], data + spaceToEnd, len - spaceToEnd);
        head = len - spaceToEnd;
    }

    // Adjust tail in case of overflow (overwrite oldest data)
    uint32_t available_space = (tail > head) ? (tail - head) : (SPP_RX_BUFFER_SIZE - (head - tail));
    if (len > available_space)
    {
        // Advance tail to maintain circular behavior
        tail = (head + 1) % SPP_RX_BUFFER_SIZE; // +1 ensures head != tail
    }
}

/**
 * @brief Pops the oldest byte from the circular buffer.
 *
 * @return The next byte in the buffer, or -1 if the buffer is empty.
 */
int BtMultiSPP::Client::pop()
{
    if (head == tail)
    {
        // Buffer is empty
        return -1;
    }
    uint8_t byte = buffer[tail];
    tail         = (tail + 1) % SPP_RX_BUFFER_SIZE;
    return byte;
}

/**
 * @brief Peeks the oldest byte from the circular buffer.
 *
 * @return The next byte in the buffer, or -1 if the buffer is empty.
 */
int BtMultiSPP::Client::peek()
{
    if (head == tail)
    {
        // Buffer is empty
        return -1;
    }

    return buffer[tail];
}
