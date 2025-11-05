/***************************************************************************
 *                                                                         *
 * Project:  MicronetToNMEA                                                *
 * Purpose:  Driver for CC1101                                             *
 * Author:   Ronan Demoment heavily based on ELECHOUSE's driver            *
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

#include "CC1101Driver.h"
#include "Micronet.h"
#include "MicronetMessageFifo.h"

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define TRANSMIT_LIST_SIZE     16
#define LOW_BANDWIDTH_VALUE    80
#define MEDIUM_BANDWIDTH_VALUE 125
#define HIGH_BANDWIDTH_VALUE   250
#define RF_TX_PROG_DELAY_US    185 // Time needed to program CC1101 before sending the first byte with CPU@24MHz

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

typedef enum
{
    RF_STATE_RX_WAIT_SYNC = 0,
    RF_STATE_RX_HEADER,
    RF_STATE_RX_PAYLOAD,
    RF_STATE_TX_TRANSMIT,
    RF_STATE_TX_LAST_TRANSMIT
} RfDriverState_t;

/***************************************************************************/
/*                               Classes                                   */
/***************************************************************************/

class RfDriver
{
  public:
    typedef enum
    {
        RF_BANDWIDTH_LOW = 0,
        RF_BANDWIDTH_MEDIUM,
        RF_BANDWIDTH_HIGH
    } RfBandwidth_t;

    RfDriver();
    virtual ~RfDriver();

    bool Init(MicronetMessageFifo *messageFifo, float frequencyOffset_mHz);
    void SetFrequencyOffset(float offset_MHz);
    void SetFrequency(float frequency_MHz);
    void SetBandwidth(RfBandwidth_t bandwidth);
    void RestartReception();
    void Transmit(MicronetMessageFifo *txMessageFifo);
    void Transmit(MicronetMessage_t *message);
    void EnableFrequencyTracking(uint32_t networkId);
    void DisableFrequencyTracking();

    void RfIsr();

  private:
    CC1101Driver             cc1101Driver;
    MicronetMessageFifo     *messageFifo;
    volatile RfDriverState_t rfState;
    MicronetMessage_t        transmitList[TRANSMIT_LIST_SIZE];
    volatile int             nextTransmitIndex;
    volatile int             messageBytesSent;
    float                    frequencyOffset_MHz;
    uint32_t                 freqTrackingNID;
    hw_timer_t              *txTimer;
    portMUX_TYPE             timerMux;

    static const uint8_t preambleAndSync[MICRONET_RF_PREAMBLE_LENGTH];

    void ScheduleTransmit();
    int  GetNextTransmitIndex();
    int  GetFreeTransmitSlot();
    void TransmitCallback();
    void RfIsr_Rx();
    void RfIsr_Tx();

    static void      TimerHandler();
    static RfDriver *rfDriver;
};


