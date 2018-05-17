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

#ifndef XCSOAR_DEVICE_DRIVER_LX_PROTOCOL_HPP
#define XCSOAR_DEVICE_DRIVER_LX_PROTOCOL_HPP

#include "Device/Port/Port.hpp"
#include "Compiler.h"

#include <stddef.h>
#include <stdint.h>

class OperationEnvironment;

namespace LX {
  static constexpr unsigned NUMTPS = 12;

  enum Command {
    PREFIX = 0x02,
    ACK = 0x06,
    SYN = 0x16,
    WRITE_FLIGHT_INFO = 0xCA,
    READ_MEMORY_SECTION = 0xcc,
    READ_FLIGHT_LIST = 0xcd,
    SEEK_MEMORY = 0xce,
    WRITE_CONTEST_CLASS = 0xD0,
    READ_LOGGER_DATA = 0xe6,
  };

#pragma pack(push, 1) // force byte alignment

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

  struct MemoryAddress32 {
    uint8_t address1;
    uint8_t address0;
    uint8_t address3;
    uint8_t address2;
  } gcc_packed;

  struct MemoryAddress24 {
    uint8_t address[3];

    MemoryAddress24 &operator=(const MemoryAddress32 &other) {
      address[0] = other.address0;
      address[1] = other.address1;
      address[2] = other.address2;
      return *this;
    }

    MemoryAddress24 &operator=(const uint8_t other[3]) {
      address[0] = other[0];
      address[1] = other[1];
      address[2] = other[2];
      return *this;
    }
  } gcc_packed;

  struct FlightInfo {
    uint8_t valid;
    MemoryAddress32 start_address;
    MemoryAddress32 end_address;
    char date[9];
    char start_time[9];
    char stop_time[9];
    uint8_t dummy0[4];
    char pilot[52];
    uint16_t logger_id;
    uint8_t flight_no; /* ? */

    bool IsValid() const {
      return valid == 1 && date[8] == 0 && start_time[8] == 0 &&
        stop_time[8] == 0;
    }
  } gcc_packed;

  struct SeekMemory {
    MemoryAddress24 start_address, end_address;
  } gcc_packed;

  struct MemorySection {
    static constexpr unsigned N = 0x10;

    uint16_t lengths[N];
  } gcc_packed;

  /**
   * Strings have extra byte for NULL.
   */
  struct ContestClass {
    char contest_class[9];
  } gcc_packed;

#pragma pack(pop)

  static inline bool
  SendSYN(Port &port)
  {
    return port.Write(SYN);
  }

  static inline bool
  ExpectACK(Port &port, OperationEnvironment &env, unsigned timeout_ms=2000)
  {
    return port.WaitForChar(ACK, env, timeout_ms) == Port::WaitResult::READY;
  }

  /**
   * Send SYN and wait for ACK.
   *
   * @return true on success
   */
  static inline bool
  Connect(Port &port, OperationEnvironment &env, unsigned timeout_ms=500)
  {
    return SendSYN(port) && ExpectACK(port, env, timeout_ms);
  }

  /**
   * Enter command mode: flush all buffers, configure a sensible
   * receive timeout, sends SYN three times and waits for ACK.
   */
  bool
  CommandMode(Port &port, OperationEnvironment &env);

  /**
   * Enter command mode without waiting for ACK.
   */
  void
  CommandModeQuick(Port &port, OperationEnvironment &env);

  static inline bool
  SendCommand(Port &port, Command command)
  {
    return port.Write(PREFIX) &&
      port.Write(command);
  }

  bool
  SendPacket(Port &port, Command command,
             const void *data, size_t length,
             OperationEnvironment &env, unsigned timeout_ms=5000);

  bool
  ReceivePacket(Port &port, Command command,
                void *data, size_t length, OperationEnvironment &env,
                unsigned first_timeout_ms, unsigned subsequent_timeout_ms,
                unsigned total_timeout_ms);

  /**
   * Wrapper for ReceivePacket() which can retry on failure.  Before
   * each retry, it performs a full handshake with the device to reset
   * its command parser.
   */
  bool
  ReceivePacketRetry(Port &port, Command command,
                     void *data, size_t length, OperationEnvironment &env,
                     unsigned first_timeout_ms, unsigned subsequent_timeout_ms,
                     unsigned total_timeout_ms,
                     unsigned n_retries);

  gcc_const
  uint8_t
  calc_crc_char(uint8_t d, uint8_t crc);

  gcc_pure
  uint8_t
  calc_crc(const void *p0, size_t len, uint8_t crc);

  bool
  ReadCRC(Port &port, void *buffer, size_t length, OperationEnvironment &env,
          unsigned first_timeout_ms, unsigned subsequent_timeout_ms,
          unsigned total_timeout_ms);

  /**
   * Writes data to a #Port, and keeps track of the CRC.
   */
  class CRCWriter {
    Port &port;
    uint8_t crc;

  public:
    CRCWriter(Port &_port):port(_port), crc(0xff) {}

    bool Write(const void *data, size_t length,
               OperationEnvironment &env, unsigned timeout_ms=5000) {
      if (!port.FullWrite(data, length, env, timeout_ms))
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
