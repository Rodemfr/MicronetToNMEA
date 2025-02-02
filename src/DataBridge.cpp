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

/***************************************************************************/
/*                              Includes                                   */
/***************************************************************************/

#include "DataBridge.h"
#include "BoardConfig.h"
#include "Globals.h"

#include <Arduino.h>
#include <string.h>

/***************************************************************************/
/*                              Constants                                  */
/***************************************************************************/

// TODO : ASCII conversion for Micronet should be done in Micronet code
const uint8_t DataBridge::asciiTable[128] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\"', ' ', ' ', '%', '&', '\'', ' ', ' ', ' ', '+', ' ', '-', '.', '/', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', ':', ' ', '<',  ' ', '>', '?', ' ', 'A',  '(', 'C', ')', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',  'W', 'X', 'Y', 'Z', ' ',  ' ', ' ', ' ', ' ', ' ', 'A', '(', 'C', ')', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',  'Q', 'R', 'S', 'T', 'U',  'V', 'W', 'X', 'Y', 'Z', ' ', ' ', ' ', ' ', ' '};

/***************************************************************************/
/*                                Macros                                   */
/***************************************************************************/

/***************************************************************************/
/*                             Local types                                 */
/***************************************************************************/

/***************************************************************************/
/*                           Local prototypes                              */
/***************************************************************************/

/***************************************************************************/
/*                               Globals                                   */
/***************************************************************************/

/***************************************************************************/
/*                              Functions                                  */
/***************************************************************************/

/*
 * @brief Constructor of the class
 *
 * @param micronetCodec Pointer to the MicronetCodec class that will be used by
 * the data bridge to handle Micronetdata
 */
DataBridge::DataBridge(MicronetCodec *micronetCodec)
{
    nmeaPlotterWriteIndex = 0;
    nmeaPlotterBuffer[0]  = 0;
    nmeaGnssWriteIndex    = 0;
    nmeaGnssBuffer[0]     = 0;
    nmeaAisWriteIndex     = 0;
    nmeaAisBuffer[0]      = 0;
    memset(&nmeaTimeStamps, 0, sizeof(nmeaTimeStamps));
    this->micronetCodec = micronetCodec;

    // Store static link configuration from BoardConfig.h
    navSourceLink     = NAV_SOURCE_LINK;
    gnssSourceLink    = GNSS_SOURCE_LINK;
    windSourceLink    = WIND_SOURCE_LINK;
    depthSourceLink   = DEPTH_SOURCE_LINK;
    speedSourceLink   = SPEED_SOURCE_LINK;
    voltageSourceLink = VOLTAGE_SOURCE_LINK;
    seaTempSourceLink = SEATEMP_SOURCE_LINK;
    compassSourceLink = COMPASS_SOURCE_LINK;

    sogFilterIndex = 0;
    memset(sogFilterBuffer, 0, SOG_COG_FILTERING_DEPTH * sizeof(float));
    cogFilterIndex = 0;
    memset(cogFilterBuffer, 0, SOG_COG_FILTERING_DEPTH * sizeof(float));
}

/*
 * @brief Desstructor of the class. Does nothing.
 */
DataBridge::~DataBridge()
{
}

/*
 * @brief Push a character in one of the NMEA queue of the bridge. This
 * function must be called each time a character is received from one of
 * the NMEA link.
 *
 * @param c Character received
 * @param sourceLink Id of the link the character was received from
 */
void DataBridge::PushNmeaChar(char c, LinkId_t sourceLink)
{
    char *nmeaBuffer     = nullptr;
    int  *nmeaWriteIndex = 0;

    // Select the appropriate stream buffer
    switch (sourceLink)
    {
    case LINK_NAV:
        nmeaBuffer     = nmeaPlotterBuffer;
        nmeaWriteIndex = &nmeaPlotterWriteIndex;
        break;
    case LINK_GNSS:
        nmeaBuffer     = nmeaGnssBuffer;
        nmeaWriteIndex = &nmeaGnssWriteIndex;
        break;
    case LINK_AIS:
        nmeaBuffer     = nmeaAisBuffer;
        nmeaWriteIndex = &nmeaAisWriteIndex;
        break;
    default:
        return;
    }

    // Check for the sentence start character
    if (((nmeaBuffer[0] != '$') && (nmeaBuffer[0] != '!')) || (c == '$') || (c == '!'))
    {
        // A new sentence started : rewind stream buffer to the first character
        nmeaBuffer[0]   = c;
        *nmeaWriteIndex = 1;
        return;
    }

    // Check if this is the last character of a sentence
    if ((*nmeaWriteIndex >= 10) && nmeaBuffer[*nmeaWriteIndex - 3] == '*')
    {
        nmeaBuffer[*nmeaWriteIndex] = 0;

        // Sanity check
        if ((*nmeaWriteIndex >= 10) && (*nmeaWriteIndex < NMEA_SENTENCE_MAX_LENGTH - 1))
        {
            // Is CRC valid
            if (IsSentenceValid(nmeaBuffer))
            {
                // Decode the sentence
                NmeaId_t sId = SentenceId(nmeaBuffer);
                switch (sId)
                {
                case NMEA_ID_RMB:
                    // Check that with received the sentence from where we expect it
                    if (sourceLink == navSourceLink)
                    {
                        DecodeRMBSentence(nmeaBuffer);
                    }
                    break;
                case NMEA_ID_RMC:
                    // Check that with received the sentence from where we expect it
                    if (sourceLink == gnssSourceLink)
                    {
                        DecodeRMCSentence(nmeaBuffer);
                        // If the sentence is not coming from the chart plotter, forward it.
                        if (sourceLink != LINK_NAV)
                        {
                            PLOTTER_NMEA.println(nmeaBuffer);
                        }
                    }
                    break;
                case NMEA_ID_GGA:
                    // Check that with received the sentence from where we expect it
                    if (sourceLink == gnssSourceLink)
                    {
                        DecodeGGASentence(nmeaBuffer);
                        // If the sentence is not coming from the chart plotter, forward it.
                        if (sourceLink != LINK_NAV)
                        {
                            PLOTTER_NMEA.println(nmeaBuffer);
                        }
                    }
                    break;
                case NMEA_ID_GLL:
                    // Check that with received the sentence from where we expect it
                    if (sourceLink == gnssSourceLink)
                    {
                        DecodeGLLSentence(nmeaBuffer);
                        // If the sentence is not coming from the chart plotter, forward it.
                        if (sourceLink != LINK_NAV)
                        {
                            PLOTTER_NMEA.println(nmeaBuffer);
                        }
                    }
                    break;
                case NMEA_ID_VTG:
                    // Check that with received the sentence from where we expect it
                    if (sourceLink == gnssSourceLink)
                    {
                        DecodeVTGSentence(nmeaBuffer);
                        // If the sentence is not coming from the chart plotter, forward it.
                        if (sourceLink != LINK_NAV)
                        {
                            PLOTTER_NMEA.println(nmeaBuffer);
                        }
                    }
                    break;
                case NMEA_ID_MWV:
                    // Check that with received the sentence from where we expect it
                    if (sourceLink == windSourceLink)
                    {
                        DecodeMWVSentence(nmeaBuffer);
                    }
                    break;
                case NMEA_ID_DPT:
                    // Check that with received the sentence from where we expect it
                    if (sourceLink == depthSourceLink)
                    {
                        DecodeDPTSentence(nmeaBuffer);
                    }
                    break;
                case NMEA_ID_VHW:
                    // Check that with received the sentence from where we expect it
                    if (sourceLink == speedSourceLink)
                    {
                        DecodeVHWSentence(nmeaBuffer);
                    }
                    break;
                case NMEA_ID_HDG:
                    // Check that with received the sentence from where we expect it
                    if (sourceLink == compassSourceLink)
                    {
                        DecodeHDGSentence(nmeaBuffer);
                    }
                    break;
                default:
                    // If an unknown sentence is received from AIS link, forward it to the chart plotter.
                    // It is especially useful to provide AIVDM/AIVDO and alert sentences.
                    if (sourceLink == LINK_AIS)
                    {
                        PLOTTER_NMEA.println(nmeaBuffer);
                    }
                    break;
                }
            }
        }
        // Reset the stream buffer
        nmeaBuffer[0]   = 0;
        *nmeaWriteIndex = 0;
        return;
    }

    // We are in the middle of a sentence : store the caracter in the stream buffer
    nmeaBuffer[*nmeaWriteIndex] = c;
    (*nmeaWriteIndex)++;

    // Check for overflow
    if (*nmeaWriteIndex >= NMEA_SENTENCE_MAX_LENGTH)
    {
        nmeaBuffer[0]   = 0;
        *nmeaWriteIndex = 0;
        return;
    }
}

/*
 * @brief Update heading & roll angle from LSM303 electronic compass. This
 * function will ignore the updated values if MicronetToNMEA has not been
 * configured to use LSM303.
 *
 * @param heading_deg Magnetic heading in degrees
 * @param roll_deg Roll angle in degrees
 */
void DataBridge::UpdateCompassData(float heading_deg, float roll_deg)
{
    // Only update compass data if the source link is LSM303
    if (COMPASS_SOURCE_LINK == LINK_COMPASS)
    {
        while (heading_deg < 0.0f)
            heading_deg += 360.0f;
        while (heading_deg >= 360.0f)
            heading_deg -= 360.0f;
        while (roll_deg < 0.0f)
            roll_deg += 360.0f;
        while (roll_deg >= 360.0f)
            roll_deg -= 360.0f;
        micronetCodec->navData.magHdg_deg.value     = heading_deg;
        micronetCodec->navData.magHdg_deg.valid     = true;
        micronetCodec->navData.magHdg_deg.timeStamp = millis();
        micronetCodec->navData.roll_deg.value       = roll_deg;
        micronetCodec->navData.roll_deg.valid       = true;
        micronetCodec->navData.roll_deg.timeStamp   = millis();
        EncodeHDG();
        EncodeRollXDR();
    }
}

/*
 * @brief Send NMEA sentences generated from updated micronet data. This
 * function checks which micronet data has been updated since last call and
 * sends the associated NMEA sentence to the concerned device.
 */
void DataBridge::SendUpdatedNMEASentences()
{
    EncodeMWV_R();
    EncodeMWV_T();
    EncodeDPT();
    EncodeMTW();
    EncodeVLW();
    EncodeVHW();
    EncodeHDG();
    EncodeBatteryXDR();
}

/*
 * @brief Updates the SOG filter with the lastest SOG raw value and returns
 * the updated filtered SOG value.
 *
 * @param newSog_kt Latest raw SOG value from GNSS
 * @return Filtered SOG value in knots
 */
float DataBridge::FilteredSOG(float newSog_kt)
{
#if (SOG_COG_FILTERING == 1)
    sogFilterBuffer[sogFilterIndex++] = newSog_kt;
    if (sogFilterIndex >= SOG_COG_FILTERING_DEPTH)
    {
        sogFilterIndex = 0;
    }

    float filteredSog_kt = sogFilterBuffer[0];
    for (int i = 1; i < SOG_COG_FILTERING_DEPTH; i++)
    {
        filteredSog_kt += sogFilterBuffer[i];
    }

    return filteredSog_kt / SOG_COG_FILTERING_DEPTH;
#else
    return newSog_kt;
#endif
}

/*
 * @brief Updates the COG filter with the lastest COG raw value and returns
 * the updated filtered COG value.
 *
 * @param newCog_deg Latest raw COG value from GNSS
 * @return Filtered COG value in knots [0..360[
 */
float DataBridge::FilteredCOG(float newCog_deg)
{
#if (SOG_COG_FILTERING == 1)
    cogFilterBuffer[cogFilterIndex++] = newCog_deg;
    if (cogFilterIndex >= SOG_COG_FILTERING_DEPTH)
    {
        cogFilterIndex = 0;
    }

    float filteredCog_deg = cogFilterBuffer[0];
    float previousCog_deg = filteredCog_deg;
    float bufferedCog_deg;
    for (int i = 1; i < SOG_COG_FILTERING_DEPTH; i++)
    {
        bufferedCog_deg = cogFilterBuffer[i];
        if (bufferedCog_deg - previousCog_deg > 180)
        {
            bufferedCog_deg -= 360;
        }
        else if (bufferedCog_deg - previousCog_deg < -180)
        {
            bufferedCog_deg += 360;
        }
        previousCog_deg = bufferedCog_deg;
        filteredCog_deg += bufferedCog_deg;
    }

    filteredCog_deg = filteredCog_deg / SOG_COG_FILTERING_DEPTH;

    if (filteredCog_deg < 0)
    {
        filteredCog_deg += 360;
    }
    else if (filteredCog_deg >= 360)
    {
        filteredCog_deg -= 360;
    }

    return filteredCog_deg;
#else
    return newCog_deg;
#endif
}

/*
 * @brief Checks if a NMEA sentence is valid (CRC & syntax).
 *
 * @param nmeaBuffer Pointer to the null terminated NMEA sentence
 * @return true if the sentence is valid, false else.
 */
bool DataBridge::IsSentenceValid(char *nmeaBuffer)
{
    if ((nmeaBuffer[0] != '$') && (nmeaBuffer[0] != '!'))
        return false;

    char *pCs = strrchr(static_cast<char *>(nmeaBuffer), '*') + 1;
    if (pCs == nullptr)
        return false;
    int16_t Cs = (NibbleValue(pCs[0]) << 4) | NibbleValue(pCs[1]);
    if (Cs < 0)
        return false;

    uint8_t crc = 0;
    for (char *pC = nmeaBuffer + 1; pC < (pCs - 1); pC++)
    {
        crc = crc ^ (*pC);
    }

    if (crc != Cs)
        return false;

    return true;
}

/*
 * @brief Retreives the ID of an NMEA sentence.
 *
 * @param nmeaBuffer Pointer to the null terminated NMEA sentence
 * @return ID of the sentence
 */
NmeaId_t DataBridge::SentenceId(char *nmeaBuffer)
{
    uint32_t sId = ((uint8_t)nmeaBuffer[3]) << 16;
    sId |= ((uint8_t)nmeaBuffer[4]) << 8;
    sId |= ((uint8_t)nmeaBuffer[5]);

    NmeaId_t nmeaSentence = NMEA_ID_UNKNOWN;

    switch (sId)
    {
    case 0x524D42:
        // RMB sentence
        nmeaSentence = NMEA_ID_RMB;
        break;
    case 0x524D43:
        // RMC sentence
        nmeaSentence = NMEA_ID_RMC;
        break;
    case 0x474741:
        // GGA sentence
        nmeaSentence = NMEA_ID_GGA;
        break;
    case 0x474C4C:
        // GLL sentence
        nmeaSentence = NMEA_ID_GLL;
        break;
    case 0x565447:
        // VTG sentence
        nmeaSentence = NMEA_ID_VTG;
        break;
    case 0x4D5756:
        // MWV sentence
        nmeaSentence = NMEA_ID_MWV;
        break;
    case 0x445054:
        // DPT sentence
        nmeaSentence = NMEA_ID_DPT;
        break;
    case 0x564857:
        // DPT sentence
        nmeaSentence = NMEA_ID_VHW;
        break;
    case 0x484447:
        // HDG sentence
        nmeaSentence = NMEA_ID_HDG;
        break;
    }

    return nmeaSentence;
}

void DataBridge::DecodeRMBSentence(char *sentence)
{
    float value;

    sentence += 7;

    if (sentence[0] != 'A')
    {
        return;
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sscanf(sentence, "%f", &value) == 1)
    {
        micronetCodec->navData.xte_nm.value     = value;
        micronetCodec->navData.xte_nm.valid     = true;
        micronetCodec->navData.xte_nm.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sentence[0] == 'R')
        micronetCodec->navData.xte_nm.value = -micronetCodec->navData.xte_nm.value;
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    memset(micronetCodec->navData.waypoint.name, ' ', sizeof(micronetCodec->navData.waypoint.name));
    if (sentence[0] != ',')
    {
        // We look for WP1 ID (target)
        uint8_t  c;
        uint32_t i;
        for (i = 0; i < sizeof(micronetCodec->navData.waypoint.name); i++)
        {
            if (sentence[i] != ',')
            {
                c = sentence[i];
                if (c < 128)
                {
                    c = asciiTable[(int32_t)sentence[i]];
                }
                else
                {
                    c = ' ';
                }
                micronetCodec->navData.waypoint.name[i]   = c;
                micronetCodec->navData.waypoint.valid     = true;
                micronetCodec->navData.waypoint.timeStamp = millis();
            }
            else
            {
                break;
            }
        }
        micronetCodec->navData.waypoint.nameLength = i;
    }
    for (int i = 0; i < 5; i++)
    {
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
    }
    if (sscanf(sentence, "%f", &value) == 1)
    {
        micronetCodec->navData.dtw_nm.value     = value;
        micronetCodec->navData.dtw_nm.valid     = true;
        micronetCodec->navData.dtw_nm.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sscanf(sentence, "%f", &value) == 1)
    {
        micronetCodec->navData.btw_deg.value     = value;
        micronetCodec->navData.btw_deg.valid     = true;
        micronetCodec->navData.btw_deg.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sscanf(sentence, "%f", &value) == 1)
    {
        micronetCodec->navData.vmgwp_kt.value     = value;
        micronetCodec->navData.vmgwp_kt.valid     = true;
        micronetCodec->navData.vmgwp_kt.timeStamp = millis();
    }
}

void DataBridge::DecodeRMCSentence(char *sentence)
{
    float degs, mins;
    float value;

    sentence += 7;

    if (sentence[0] != ',')
    {
        micronetCodec->navData.time.hour      = (sentence[0] - '0') * 10 + (sentence[1] - '0');
        micronetCodec->navData.time.minute    = (sentence[2] - '0') * 10 + (sentence[3] - '0');
        micronetCodec->navData.time.valid     = true;
        micronetCodec->navData.time.timeStamp = millis();
    }

    for (int i = 0; i < 2; i++)
    {
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
    }

    if (sentence[0] != ',')
    {
        degs = (sentence[0] - '0') * 10 + (sentence[1] - '0');
        sscanf(sentence + 2, "%f,", &mins);
        micronetCodec->navData.latitude_deg.value = degs + mins / 60.0f;
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
        if (sentence[0] == 'S')
            micronetCodec->navData.latitude_deg.value = -micronetCodec->navData.latitude_deg.value;
        micronetCodec->navData.latitude_deg.valid     = true;
        micronetCodec->navData.latitude_deg.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;

    if (sentence[0] != ',')
    {
        degs = (sentence[0] - '0') * 100 + (sentence[1] - '0') * 10 + (sentence[2] - '0');
        sscanf(sentence + 3, "%f,", &mins);
        micronetCodec->navData.longitude_deg.value = degs + mins / 60.0f;
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
        if (sentence[0] == 'W')
            micronetCodec->navData.longitude_deg.value = -micronetCodec->navData.longitude_deg.value;
        micronetCodec->navData.longitude_deg.valid     = true;
        micronetCodec->navData.longitude_deg.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;

    if (sscanf(sentence, "%f", &value) == 1)
    {
        micronetCodec->navData.sog_kt.value     = FilteredSOG(value);
        micronetCodec->navData.sog_kt.valid     = true;
        micronetCodec->navData.sog_kt.timeStamp = millis();

#if (EMULATE_SPD_WITH_SOG == 1)
        micronetCodec->navData.spd_kt.value     = FilteredSOG(value);
        micronetCodec->navData.spd_kt.valid     = true;
        micronetCodec->navData.spd_kt.timeStamp = millis();
#endif
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;

    if (sscanf(sentence, "%f", &value) == 1)
    {
        if (value < 0)
            value += 360.0f;

        micronetCodec->navData.cog_deg.value     = FilteredCOG(value);
        micronetCodec->navData.cog_deg.valid     = true;
        micronetCodec->navData.cog_deg.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;

    if (sentence[0] != ',')
    {
        micronetCodec->navData.date.day       = (sentence[0] - '0') * 10 + (sentence[1] - '0');
        micronetCodec->navData.date.month     = (sentence[2] - '0') * 10 + (sentence[3] - '0');
        micronetCodec->navData.date.year      = (sentence[4] - '0') * 10 + (sentence[5] - '0');
        micronetCodec->navData.date.valid     = true;
        micronetCodec->navData.date.timeStamp = millis();
    }
}

void DataBridge::DecodeGGASentence(char *sentence)
{
    float degs, mins;

    sentence += 7;

    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;

    if (sentence[0] != ',')
    {
        degs = (sentence[0] - '0') * 10 + (sentence[1] - '0');
        sscanf(sentence + 2, "%f,", &mins);
        micronetCodec->navData.latitude_deg.value = degs + mins / 60.0f;
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
        if (sentence[0] == 'S')
            micronetCodec->navData.latitude_deg.value = -micronetCodec->navData.latitude_deg.value;
        micronetCodec->navData.latitude_deg.valid     = true;
        micronetCodec->navData.latitude_deg.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sentence[0] != ',')
    {
        degs = (sentence[0] - '0') * 100 + (sentence[1] - '0') * 10 + (sentence[2] - '0');
        sscanf(sentence + 3, "%f,", &mins);
        micronetCodec->navData.longitude_deg.value = degs + mins / 60.0f;
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
        if (sentence[0] == 'W')
            micronetCodec->navData.longitude_deg.value = -micronetCodec->navData.longitude_deg.value;
        micronetCodec->navData.longitude_deg.valid     = true;
        micronetCodec->navData.longitude_deg.timeStamp = millis();
    }
}

void DataBridge::DecodeGLLSentence(char *sentence)
{
    float degs, mins;

    sentence += 7;

    if (sentence[0] != ',')
    {
        degs = (sentence[0] - '0') * 10 + (sentence[1] - '0');
        sscanf(sentence + 2, "%f,", &mins);
        micronetCodec->navData.latitude_deg.value = degs + mins / 60.0f;
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
        if (sentence[0] == 'S')
            micronetCodec->navData.latitude_deg.value = -micronetCodec->navData.latitude_deg.value;
        micronetCodec->navData.latitude_deg.valid     = true;
        micronetCodec->navData.latitude_deg.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sentence[0] != ',')
    {
        degs = (sentence[0] - '0') * 100 + (sentence[1] - '0') * 10 + (sentence[2] - '0');
        sscanf(sentence + 3, "%f,", &mins);
        micronetCodec->navData.longitude_deg.value = degs + mins / 60.0f;
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
        if (sentence[0] == 'W')
            micronetCodec->navData.longitude_deg.value = -micronetCodec->navData.longitude_deg.value;
        micronetCodec->navData.longitude_deg.valid     = true;
        micronetCodec->navData.longitude_deg.timeStamp = millis();
    }
}

void DataBridge::DecodeVTGSentence(char *sentence)
{
    float value;
    int   comaCount = 0;
    char *pChar     = sentence;
    int   fieldsToSkip;

    sentence += 7;

    // Here we check which version of VTG sentence we received
    // older devices might send a sentence without the T, M and N characters
    while ((pChar = strchr(pChar + 1, ',')) != nullptr)
    {
        comaCount++;
    }

    if (comaCount == 4)
    {
        // Version without T, M & N
        fieldsToSkip = 2;
    }
    else
    {
        // Correct version
        fieldsToSkip = 4;
    }

    if (sscanf(sentence, "%f", &value) == 1)
    {
        if (value < 0)
            value += 360.0f;

        micronetCodec->navData.cog_deg.value     = FilteredCOG(value);
        micronetCodec->navData.cog_deg.valid     = true;
        micronetCodec->navData.cog_deg.timeStamp = millis();
    }
    for (int i = 0; i < fieldsToSkip; i++)
    {
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
    }
    if (sscanf(sentence, "%f", &value) == 1)
    {
        micronetCodec->navData.sog_kt.value     = FilteredSOG(value);
        micronetCodec->navData.sog_kt.valid     = true;
        micronetCodec->navData.sog_kt.timeStamp = millis();

#if (EMULATE_SPD_WITH_SOG == 1)
        micronetCodec->navData.spd_kt.value     = FilteredSOG(value);
        micronetCodec->navData.spd_kt.valid     = true;
        micronetCodec->navData.spd_kt.timeStamp = millis();
#endif
    }
}

void DataBridge::DecodeMWVSentence(char *sentence)
{
    float value;
    float awa = -9999.0;
    float aws;

    sentence += 7;

    if (sscanf(sentence, "%f", &value) == 1)
    {
        awa = value;
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sentence[0] != 'R')
    {
        return;
    }
    if (awa > -9000.0)
    {
        if (awa > 180.0)
            awa -= 360.0f;
        micronetCodec->navData.awa_deg.value     = awa;
        micronetCodec->navData.awa_deg.valid     = true;
        micronetCodec->navData.awa_deg.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sscanf(sentence, "%f", &value) == 1)
    {
        aws = value;
    }
    else
        return;
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    switch (sentence[0])
    {
    case 'M':
        aws *= 1.943844;
        break;
    case 'K':
        aws *= 0.5399568;
        break;
    case 'N':
        break;
    default:
        return;
    }

    micronetCodec->navData.aws_kt.value     = aws;
    micronetCodec->navData.aws_kt.valid     = true;
    micronetCodec->navData.aws_kt.timeStamp = millis();
    micronetCodec->CalculateTrueWind();
}

void DataBridge::DecodeDPTSentence(char *sentence)
{
    float value;
    float depth;

    sentence += 7;

    if (sscanf(sentence, "%f", &value) == 1)
    {
        depth = value;
    }
    else
        return;
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sscanf(sentence, "%f", &value) == 1)
    {
        micronetCodec->navData.dpt_m.value     = depth + value;
        micronetCodec->navData.dpt_m.valid     = true;
        micronetCodec->navData.dpt_m.timeStamp = millis();
    }
}

void DataBridge::DecodeVHWSentence(char *sentence)
{
    float value;

    sentence += 7;

    for (int i = 0; i < 2; i++)
    {
        if ((sentence = strchr(sentence, ',')) == nullptr)
            return;
        sentence++;
    }
    if (sscanf(sentence, "%f", &value) != 1)
        return;
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sentence[0] == 'M')
    {
        if (value < 0)
            value += 360.0f;
        micronetCodec->navData.magHdg_deg.value     = value;
        micronetCodec->navData.magHdg_deg.valid     = true;
        micronetCodec->navData.magHdg_deg.timeStamp = millis();
    }
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sscanf(sentence, "%f", &value) != 1)
        return;
    if ((sentence = strchr(sentence, ',')) == nullptr)
        return;
    sentence++;
    if (sentence[0] == 'N')
    {
        micronetCodec->navData.spd_kt.value     = value;
        micronetCodec->navData.spd_kt.valid     = true;
        micronetCodec->navData.spd_kt.timeStamp = millis();
    }
}

void DataBridge::DecodeHDGSentence(char *sentence)
{
    float value;

    sentence += 7;

    if (sscanf(sentence, "%f", &value) != 1)
        return;
    while (value < 0)
        value += 360.0f;
    while (value >= 360.0)
        value -= 360.0f;
    micronetCodec->navData.magHdg_deg.value     = value;
    micronetCodec->navData.magHdg_deg.valid     = true;
    micronetCodec->navData.magHdg_deg.timeStamp = millis();
}

int16_t DataBridge::NibbleValue(char c)
{
    if ((c >= '0') && (c <= '9'))
    {
        return (c - '0');
    }
    else if ((c >= 'A') && (c <= 'F'))
    {
        return (c - 'A') + 0x0a;
    }
    else if ((c >= 'a') && (c <= 'f'))
    {
        return (c - 'a') + 0x0a;
    }

    return -1;
}

void DataBridge::EncodeMWV_R()
{
    if (WIND_SOURCE_LINK == LINK_MICRONET)
    {
        bool update;

        update = (micronetCodec->navData.awa_deg.timeStamp > nmeaTimeStamps.vwr + NMEA_SENTENCE_MIN_PERIOD_MS);
        update = update && (micronetCodec->navData.aws_kt.timeStamp > nmeaTimeStamps.vwr + NMEA_SENTENCE_MIN_PERIOD_MS);
        update = update && (micronetCodec->navData.awa_deg.valid && micronetCodec->navData.aws_kt.valid);

        if (update)
        {
            char  sentence[NMEA_SENTENCE_MAX_LENGTH];
            float absAwa = micronetCodec->navData.awa_deg.value;
            if (absAwa < 0.0f)
                absAwa += 360.0f;
            sprintf(sentence, "$INMWV,%.1f,R,%.1f,N,A", absAwa, micronetCodec->navData.aws_kt.value);
            AddNmeaChecksum(sentence);
            nmeaTimeStamps.vwr = millis();
            PLOTTER_NMEA.println(sentence);
        }
    }
}

void DataBridge::EncodeMWV_T()
{
    if (WIND_SOURCE_LINK == LINK_MICRONET)
    {
        bool update;

        update = (micronetCodec->navData.twa_deg.timeStamp > nmeaTimeStamps.vwt + NMEA_SENTENCE_MIN_PERIOD_MS);
        update = update && (micronetCodec->navData.tws_kt.timeStamp > nmeaTimeStamps.vwt + NMEA_SENTENCE_MIN_PERIOD_MS);
        update = update && (micronetCodec->navData.twa_deg.valid && micronetCodec->navData.tws_kt.valid);

        if (update)
        {
            char  sentence[NMEA_SENTENCE_MAX_LENGTH];
            float absTwa = micronetCodec->navData.twa_deg.value;
            if (absTwa < 0.0f)
                absTwa += 360.0f;
            sprintf(sentence, "$INMWV,%.1f,T,%.1f,N,A", absTwa, micronetCodec->navData.tws_kt.value);
            AddNmeaChecksum(sentence);
            nmeaTimeStamps.vwt = millis();
            PLOTTER_NMEA.println(sentence);
        }
    }
}

void DataBridge::EncodeDPT()
{
    if (DEPTH_SOURCE_LINK == LINK_MICRONET)
    {
        bool update;

        update = (micronetCodec->navData.dpt_m.timeStamp > nmeaTimeStamps.dpt + NMEA_SENTENCE_MIN_PERIOD_MS);
        update = update && micronetCodec->navData.dpt_m.valid;

        if (update)
        {
            char sentence[NMEA_SENTENCE_MAX_LENGTH];
            sprintf(sentence, "$INDPT,%.1f,%.1f,", micronetCodec->navData.dpt_m.value, micronetCodec->navData.depthOffset_m);
            AddNmeaChecksum(sentence);
            nmeaTimeStamps.dpt = millis();
            PLOTTER_NMEA.println(sentence);
        }
    }
}

void DataBridge::EncodeMTW()
{
    if (SEATEMP_SOURCE_LINK == LINK_MICRONET)
    {
        bool update;

        update = (micronetCodec->navData.stp_degc.timeStamp > (nmeaTimeStamps.mtw + NMEA_SENTENCE_MIN_PERIOD_MS));
        update = update && micronetCodec->navData.stp_degc.valid;

        if (update)
        {
            char sentence[NMEA_SENTENCE_MAX_LENGTH];
            sprintf(sentence, "$INMTW,%.1f,C", micronetCodec->navData.stp_degc.value);
            AddNmeaChecksum(sentence);
            nmeaTimeStamps.mtw = millis();
            PLOTTER_NMEA.println(sentence);
        }
    }
}

void DataBridge::EncodeVLW()
{
    if (SPEED_SOURCE_LINK == LINK_MICRONET)
    {
        bool update;

        update = (micronetCodec->navData.log_nm.timeStamp > nmeaTimeStamps.vlw + NMEA_SENTENCE_MIN_PERIOD_MS);
        update = update && (micronetCodec->navData.trip_nm.timeStamp > nmeaTimeStamps.vlw + NMEA_SENTENCE_MIN_PERIOD_MS);
        update = update && (micronetCodec->navData.log_nm.valid && micronetCodec->navData.trip_nm.valid);

        if (update)
        {
            char sentence[NMEA_SENTENCE_MAX_LENGTH];
            sprintf(sentence, "$INVLW,%.1f,N,%.1f,N,,N,,N", micronetCodec->navData.log_nm.value, micronetCodec->navData.trip_nm.value);
            AddNmeaChecksum(sentence);
            nmeaTimeStamps.vlw = millis();
            PLOTTER_NMEA.println(sentence);
        }
    }
}

void DataBridge::EncodeVHW()
{
    if (SPEED_SOURCE_LINK == LINK_MICRONET)
    {
        bool update =
            (micronetCodec->navData.spd_kt.timeStamp > nmeaTimeStamps.vhw + NMEA_SENTENCE_MIN_PERIOD_MS) && (micronetCodec->navData.spd_kt.valid);

        if (update)
        {
            char sentence[NMEA_SENTENCE_MAX_LENGTH];
            if ((micronetCodec->navData.magHdg_deg.valid) && (micronetCodec->navData.spd_kt.valid))
            {
                float trueHeading = micronetCodec->navData.magHdg_deg.value + micronetCodec->navData.magneticVariation_deg;
                if (trueHeading < 0.0f)
                {
                    trueHeading += 360.0f;
                }
                if (trueHeading >= 360.0f)
                {
                    trueHeading -= 360.0f;
                }
                sprintf(sentence, "$INVHW,%.1f,T,%.1f,M,%.1f,N,,K", trueHeading, micronetCodec->navData.magHdg_deg.value,
                        micronetCodec->navData.spd_kt.value);
            }
            else if (micronetCodec->navData.spd_kt.valid)
            {
                sprintf(sentence, "$INVHW,,T,,M,%.1f,N,,K", micronetCodec->navData.spd_kt.value);
            }
            AddNmeaChecksum(sentence);
            nmeaTimeStamps.vhw = millis();
            PLOTTER_NMEA.println(sentence);
        }
    }
}

void DataBridge::EncodeHDG()
{
    if ((COMPASS_SOURCE_LINK == LINK_MICRONET) || (COMPASS_SOURCE_LINK == LINK_COMPASS))
    {
        bool update;

        update = (micronetCodec->navData.magHdg_deg.timeStamp > nmeaTimeStamps.hdg + NMEA_SENTENCE_MIN_PERIOD_MS);
        update = update && micronetCodec->navData.magHdg_deg.valid;

        if (update)
        {
            char sentence[NMEA_SENTENCE_MAX_LENGTH];
            sprintf(sentence, "$INHDG,%.1f,0,E,%.1f,%c", micronetCodec->navData.magHdg_deg.value, fabsf(micronetCodec->navData.magneticVariation_deg),
                    (micronetCodec->navData.magneticVariation_deg < 0.0f) ? 'W' : 'E');
            AddNmeaChecksum(sentence);
            nmeaTimeStamps.hdg = millis();
            PLOTTER_NMEA.println(sentence);
        }
    }
}

void DataBridge::EncodeBatteryXDR()
{
    if (VOLTAGE_SOURCE_LINK == LINK_MICRONET)
    {
        bool update;

        update = (micronetCodec->navData.vcc_v.timeStamp > nmeaTimeStamps.vcc + NMEA_SENTENCE_MIN_PERIOD_MS);
        update = update && micronetCodec->navData.vcc_v.valid;

        if (update)
        {
            char sentence[NMEA_SENTENCE_MAX_LENGTH];
            sprintf(sentence, "$INXDR,U,%.1f,V,TACKTICK#0", micronetCodec->navData.vcc_v.value);
            AddNmeaChecksum(sentence);
            nmeaTimeStamps.vcc = millis();
            PLOTTER_NMEA.println(sentence);
        }
    }
}

void DataBridge::EncodeRollXDR()
{
    bool update;

    update = (micronetCodec->navData.roll_deg.timeStamp > nmeaTimeStamps.roll + NMEA_SENTENCE_MIN_PERIOD_MS);
    update = update && micronetCodec->navData.roll_deg.valid;

    if (update)
    {
        char sentence[NMEA_SENTENCE_MAX_LENGTH];
        sprintf(sentence, "$INXDR,A,%.0f,D,ROLL", micronetCodec->navData.roll_deg.value);
        AddNmeaChecksum(sentence);
        nmeaTimeStamps.roll = millis();
        PLOTTER_NMEA.println(sentence);
    }
}

uint8_t DataBridge::AddNmeaChecksum(char *sentence)
{
    uint8_t crc = 0;
    char    crcString[8];
    char   *pChar = sentence + 1;

    while (*pChar != 0)
    {
        crc ^= (*pChar);
        pChar++;
    }

    sprintf(crcString, "*%02x", crc);
    strcat(sentence, crcString);

    return crc;
}
