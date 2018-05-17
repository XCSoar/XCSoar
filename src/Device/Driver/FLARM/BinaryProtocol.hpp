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

#ifndef XCSOAR_FLARM_BINARY_PROTOCOL_HPP
#define XCSOAR_FLARM_BINARY_PROTOCOL_HPP

#include "OS/ByteOrder.hpp"
#include "Compiler.h"

#include <type_traits>

#include <stdint.h>
#include <stddef.h>

class Port;
struct Declaration;
class OperationEnvironment;
class RecordedFlightList;
struct RecordedFlightInfo;

namespace FLARM {
  static constexpr uint8_t START_FRAME = 0x73;
  static constexpr uint8_t ESCAPE = 0x78;
  static constexpr uint8_t ESCAPE_ESCAPE = 0x55;
  static constexpr uint8_t ESCAPE_START = 0x31;

  enum MessageType {
    MT_ERROR = 0x00,
    MT_ACK = 0xA0,
    MT_NACK = 0xB7,
    MT_PING = 0x01,
    MT_SETBAUDRATE = 0x02,
    MT_FLASHUPLOAD = 0x10,
    MT_EXIT = 0x12,
    MT_SELECTRECORD = 0x20,
    MT_GETRECORDINFO = 0x21,
    MT_GETIGCDATA = 0x22,
  };

  /**
   * The binary transfer mode works with "frames". Each frame consists of a
   * start byte (0x73), an 8-byte frame header and an optional payload. The
   * length of the payload is transfered inside the frame header.
   */
  struct FrameHeader
  {
    /**
     * Length of the frame header (8) + length of the payload in bytes.
     * Use the Get/Set() functions to interact with this attribute!
     */
    PackedLE16 length;

    /**
     * Protocol version. Frames with higher version number than implemented
     * by software shall be discarded.
     */
    uint8_t version;

    /**
     * Sequence counter. Shall be increased by one for every frame sent.
     * Use the Get/Set() functions to interact with this attribute!
     */
    PackedLE16 sequence_number;

    /** Message type */
    uint8_t type;

    /**
     * CRC over the complete message, except CRC field.
     * Use the Get/Set() functions to interact with this attribute!
     */
    PackedLE16 crc;
  };

  static_assert(sizeof(FrameHeader) == 8,
                "The FrameHeader struct needs to have a size of 8 bytes");
  static_assert(alignof(FrameHeader) == 1, "Wrong alignment");
  static_assert(std::is_trivial<FrameHeader>::value, "type is not trivial");

  /**
   * Convenience function. Returns a pre-populated FrameHeader instance that is
   * ready to be sent by the SendFrameHeader() function.
   * @param message_type Message type of the FrameHeader
   * @param data Optional pointer to the first byte of the payload. Used for
   * CRC calculations.
   * @param length Optional length of the payload
   * @return An initialized FrameHeader instance
   */
  FrameHeader PrepareFrameHeader(unsigned sequence_number,
                                 MessageType message_type,
                                 const void *data = nullptr,
                                 size_t length = 0);

  /**
   * Sends the specified data stream to the FLARM using the escaping algorithm
   * specified in the reference document.
   * @param data Pointer to the first byte
   * @param length Amount of bytes that should be send. Note that the actual
   * number of bytes can be larger due to the escaping.
   * @param timeout_ms Timeout in milliseconds
   * @return True if the data was sent successfully, False if a timeout
   * or some transfer problems occurred
   */
  bool SendEscaped(Port &port, const void *buffer, size_t length,
                   OperationEnvironment &env, unsigned timeout_ms);

  /**
   * Reads a specified number of bytes from the port while applying the
   * escaping algorithm. The algorithm will try to read bytes until the
   * specified number is reached or a timeout occurs.
   * @param data Pointer to the first byte of the writable buffer
   * @param length Length of the buffer that should be filled
   * @param timeout_ms Timeout in milliseconds
   * @return True if the data was received successfully, False if a timeout
   * or any transfer problems occurred
   */
  bool ReceiveEscaped(Port &port, void *data, size_t length,
                      OperationEnvironment &env, unsigned timeout_ms);
};

#endif
