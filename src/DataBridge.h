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

#ifndef DATABRIDGE_H_
#define DATABRIDGE_H_

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "MicronetCodec.h"
#include "NavigationData.h"

#include <stdint.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

#define NMEA_SENTENCE_MAX_LENGTH   128
#define NMEA_SENTENCE_HISTORY_SIZE 24

/***************************************************************************/
/*                                Types                                    */
/***************************************************************************/

typedef enum
{
    LINK_NAV,
    LINK_GNSS,
    LINK_MICRONET,
    LINK_COMPASS,
    LINK_AIS
} LinkId_t;

typedef enum
{
    NMEA_ID_UNKNOWN,
    NMEA_ID_RMB,
    NMEA_ID_RMC,
    NMEA_ID_GGA,
    NMEA_ID_GLL,
    NMEA_ID_VTG,
    NMEA_ID_MWV,
    NMEA_ID_DPT,
    NMEA_ID_VHW,
    NMEA_ID_HDG
} NmeaId_t;

typedef struct
{
    uint32_t vwr;
    uint32_t vwt;
    uint32_t dpt;
    uint32_t mtw;
    uint32_t vlw;
    uint32_t vhw;
    uint32_t hdg;
    uint32_t vcc;
    uint32_t roll;
} NmeaTimeStamps_t;

#define NMEA_SENTENCE_MIN_PERIOD_MS 1000

class DataBridge
{
  public:
    DataBridge(MicronetCodec *micronetCodec);
    virtual ~DataBridge();

    void PushNmeaChar(char c, LinkId_t sourceLink);
    void UpdateCompassData(float heading_deg, float heel_deg);
    void UpdateMicronetData();

  private:
    static const uint8_t asciiTable[128];
    char                 nmeaExtBuffer[NMEA_SENTENCE_MAX_LENGTH];
    char                 nmeaGnssBuffer[NMEA_SENTENCE_MAX_LENGTH];
    int                  nmeaExtWriteIndex;
    int                  nmeaGnssWriteIndex;
    NmeaTimeStamps_t     nmeaTimeStamps;
    LinkId_t             navSourceLink;
    LinkId_t             gnssSourceLink;
    LinkId_t             windSourceLink;
    LinkId_t             depthSourceLink;
    LinkId_t             speedSourceLink;
    LinkId_t             voltageSourceLink;
    LinkId_t             seaTempSourceLink;
    LinkId_t             compassSourceLink;
    MicronetCodec       *micronetCodec;
    int                  sogFilterIndex;
    float                sogFilterBuffer[SOG_COG_FILTERING_DEPTH];
    int                  cogFilterIndex;
    float                cogFilterBuffer[SOG_COG_FILTERING_DEPTH];

    float FilteredSOG(float newSog_kt);
    float FilteredCOG(float newCog_deg);

    bool     IsSentenceValid(char *nmeaBuffer);
    NmeaId_t SentenceId(char *nmeaBuffer);
    void     DecodeRMBSentence(char *sentence);
    void     DecodeRMCSentence(char *sentence);
    void     DecodeGGASentence(char *sentence);
    void     DecodeGLLSentence(char *sentence);
    void     DecodeVTGSentence(char *sentence);
    void     DecodeMWVSentence(char *sentence);
    void     DecodeDPTSentence(char *sentence);
    void     DecodeVHWSentence(char *sentence);
    void     DecodeHDGSentence(char *sentence);
    int16_t  NibbleValue(char c);

    void EncodeMWV_R();
    void EncodeMWV_T();
    void EncodeDPT();
    void EncodeMTW();
    void EncodeVLW();
    void EncodeVHW();
    void EncodeHDG();
    void EncodeBatteryXDR();
    void EncodeRollXDR();

    uint8_t AddNmeaChecksum(char *sentence);
};

/***************************************************************************/
/*                              Prototypes                                 */
/***************************************************************************/

#endif /* DATABRIDGE_H_ */
