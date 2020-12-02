/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_FANET_DEVICE_HPP
#define XCSOAR_FANET_DEVICE_HPP

#include "Device/Driver.hpp"
#include "FANET/List.hpp"
#include "FANET/FanetStation.hpp"

struct NMEAInfo;

class FanetDevice final : public AbstractDevice {
private:
  enum FanetPacketType {
    FANET_TYPE_ACK,
    FANET_TYPE_TRACKING,
    FANET_TYPE_NAME,
    FANET_TYPE_MESSAGE,
    FANET_TYPE_SERVICE,
    FANET_TYPE_LANDMARKS,
    FANET_TYPE_REMOTE_CONFIG,
    FANET_TYPE_GROUND_TRACKING,
    FANET_TYPE_HW_INFO,
    FANET_TYPE_THERMAL
  };

  enum FanetServiceHeaderBits {
    FANET_SERVICE_BIT_EHEADER,
    FANET_SERVICE_BIT_SOC,
    FANET_SERVICE_BIT_REMOTECFGSUPPORT,
    FANET_SERVICE_BIT_PRESSURE,
    FANET_SERVICE_BIT_HUMIDITY,
    FANET_SERVICE_BIT_WIND,
    FANET_SERVICE_BIT_TEMP,
    FANET_SERVICE_BIT_INET
  };

  /**
   * Checks if nth bit is set
   *
   * @return true if its set, false otherwise
   */
  static bool IsNthBitSet(uint8_t byte, int bit) { return byte & (1 << bit); };

  /**
   * Decodes FANET service(type 4) payload and updates provided station with
   * decoded info
   *
   * @param payload to deode
   * @param payload_length
   * @param station to update
   * @return true if successfully decoded, false otherwise
   */
  static bool DecodeServicePayload(uint8_t *payload, int16_t payload_length,
                                   FanetStation *station);


  /**
   * Decodes first 6 bytes of provided payload to GeoPoint,
   * does not check for length!
   *
   * @param buf payload(>= 6byte) to decode
   * @return GeoPoint with decoded location if successfull,
   *    GeoPoint::Invalid() otherwise
   */
  static GeoPoint Payload2GeoPoint(const uint8_t *buf);

  /**
   * Converts last bits 0-6 of byte to float. if bit 7 is set multiplies it
   * with scale
   *
   * @param byte payload to convert
   * @param scale scale to apply if bit 7 is 1
   * @return value*scale if bit 7 set, value otherwise
   */
  static float Payload2Float(uint8_t byte, float scale);

  /**
   * Parses #FNF line
   *
   * @param stations_list list of stations to add parsed station to
   * @param clock current time
   * @return true if packet is parsed fully
   */
  static bool ParseRxPacket(char *cmd_str, StationsList &stations_list,
                            double clock);

  /**
   * Updates existing station or allocates a new one in stations_list
   *
   * @param stations_list list to add station to
   * @param clock current time
   * @param station station to add
   * @return true if station is successfully added
   */
  static bool AddStationToList(StationsList &stations_list, double clock,
                               FanetStation station);

  /**
   * Replace first occurrence of newline character with \0 and moves pointer
   * to the first non space character
   *
   * @param cmd_str line to replace newline character in
   * @return stripped cmd_str
   */
  static char *RemoveExtraChars(char *cmd_str);

  /**
   * Check if cmd_str contains only hexadecimal chars & commas and is > 0 chars
   *
   * @param cmd_str line to check
   * @return true if check is passed, false otherwise
   */
  static bool IntegrityCheck(char *cmd_str);

public:
  // Virtual method from class Device
  bool ParseNMEA(const char *line, NMEAInfo &info) override;
};

#endif //XCSOAR_FANET_DEVICE_HPP
