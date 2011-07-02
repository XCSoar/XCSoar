/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_DRIVER_LX_PROTOCOL_HPP
#define XCSOAR_DEVICE_DRIVER_LX_PROTOCOL_HPP

#include "Device/Port.hpp"
#include "Compiler.h"

#include <stddef.h>
#include <stdint.h>

namespace LX {
  static const unsigned int NUMTPS = 12;

  enum command {
    PREFIX = 0x02,
    ACK = 0x06,
    SYN = 0x16,
    WRITE_FLIGHT_INFO = 0xCA,
    WRITE_CONTEST_CLASS = 0xD0,
  };

  static const char LX_ACK_STRING[] = { ACK, 0 };

  /**
   * Strings have extra byte for NULL.
   */
  struct Pilot {
    uint8_t unknown1[3];
    char PilotName[19];
    char GliderType[12];
    char GliderID[8];
    char CompetitionID[4];
    uint8_t unknown2[73];
  } gcc_packed;

  /**
   * Strings have extra byte for NULL.
   */
  struct Declaration {
    uint8_t unknown1[5];
    uint8_t dayinput;
    uint8_t monthinput;
    uint8_t yearinput;
    uint8_t dayuser;
    uint8_t monthuser;
    uint8_t yearuser;
    int16_t taskid;
    uint8_t numtps;
    uint8_t tptypes[NUMTPS];
    int32_t Longitudes[NUMTPS];
    int32_t Latitudes[NUMTPS];
    char WaypointNames[NUMTPS][9];
  } gcc_packed;

  /**
   * Strings have extra byte for NULL.
   */
  struct ContestClass {
    char contest_class[9];
  } gcc_packed;

  static inline bool
  ExpectACK(Port &port)
  {
    return port.ExpectString(LX_ACK_STRING);
  }

  /**
   * Send SYN and wait for ACK.
   *
   * @return true on success
   */
  static inline bool
  Connect(Port &port)
  {
    return port.Write(SYN) && ExpectACK(port);
  }

  static inline bool
  SendCommand(Port &port, enum command command)
  {
    return port.Write(PREFIX) &&
      port.Write(command);
  }

  gcc_const
  uint8_t
  calc_crc_char(uint8_t d, uint8_t crc);

  gcc_pure
  uint8_t
  calc_crc(const void *p0, size_t len, uint8_t crc);

  /**
   * Writes data to a #Port, and keeps track of the CRC.
   */
  class CRCWriter {
    Port &port;
    uint8_t crc;

  public:
    CRCWriter(Port &_port):port(_port), crc(0xff) {}

    bool Write(const void *data, size_t length) {
      if (!port.Write(data, length))
        return false;

      crc = calc_crc((const uint8_t *)data, length, crc);
      return true;
    }

    bool Write(uint8_t value) {
      if (!port.Write(value))
        return false;

      crc = calc_crc_char(value, crc);
      return true;
    }

    /**
     * Write the CRC, and reset it, so the object can be reused.
     */
    bool Flush() {
      bool success = port.Write(crc);
      crc = 0xff;
      return success;
    }
  };
}

#endif
