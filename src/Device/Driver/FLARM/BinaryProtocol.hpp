// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/PackedLittleEndian.hxx"

#include <chrono>
#include <cstdint>
#include <cstddef>
#include <span>
#include <type_traits>

/* damn you, windows.h! */
#ifdef ERROR
#undef ERROR
#endif

class Port;
struct Declaration;
class OperationEnvironment;
class RecordedFlightList;
struct RecordedFlightInfo;

namespace FLARM {

static constexpr std::byte START_FRAME{0x73};
static constexpr std::byte ESCAPE{0x78};
static constexpr std::byte ESCAPE_ESCAPE{0x55};
static constexpr std::byte ESCAPE_START{0x31};

enum class MessageType : uint8_t {
  ERROR = 0x00,
  ACK = 0xA0,
  NACK = 0xB7,
  PING = 0x01,
  SETBAUDRATE = 0x02,
  FLASHUPLOAD = 0x10,
  EXIT = 0x12,
  SELECTRECORD = 0x20,
  GETRECORDINFO = 0x21,
  GETIGCDATA = 0x22,
};

/**
 * The binary transfer mode works with "frames". Each frame consists of a
 * start byte (0x73), an 8-byte frame header and an optional payload. The
 * length of the payload is transfered inside the frame header.
 */
struct FrameHeader {
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
  MessageType type;

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
 * @param payload the payload; used for CRC calculations
 * @return An initialized FrameHeader instance
 */
FrameHeader
PrepareFrameHeader(unsigned sequence_number,
                   MessageType message_type,
                   std::span<const std::byte> payload={}) noexcept;

/**
 * Sends the specified data stream to the FLARM using the escaping algorithm
 * specified in the reference document.
 * @param data Pointer to the first byte
 * @param length Amount of bytes that should be send. Note that the actual
 * number of bytes can be larger due to the escaping.
 */
void
SendEscaped(Port &port, std::span<const std::byte> src,
            OperationEnvironment &env,
            std::chrono::steady_clock::duration timeout);

/**
 * Reads a specified number of bytes from the port while applying the
 * escaping algorithm. The algorithm will try to read bytes until the
 * specified number is reached or a timeout occurs.
 * @param data Pointer to the first byte of the writable buffer
 * @param length Length of the buffer that should be filled
 * @return True if the data was received successfully, False if a timeout
 * or any transfer problems occurred
 */
bool
ReceiveEscaped(Port &port, std::span<std::byte> dest,
               OperationEnvironment &env,
               std::chrono::steady_clock::duration timeout);

} // namespace FLARM

