// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "CRC16.hpp"
#include "Device/Error.hpp"
#include "Device/Port/Port.hpp"
#include "time/TimeoutClock.hpp"
#include "util/SpanCast.hxx"

#include <algorithm> // for std::find_if()

static constexpr auto
FindSpecial(std::span<const std::byte>::iterator begin,
            std::span<const std::byte>::iterator end) noexcept
{
  return std::find_if(begin, end, [](std::byte b){
    return b == FLARM::START_FRAME || b == FLARM::ESCAPE;
  });
}

/* kludge because several constructor overloads are missing in Apple
   Xcode */
static constexpr std::span<const std::byte>
MakeSpan(typename std::span<const std::byte>::iterator begin,
         typename std::span<const std::byte>::iterator end) noexcept
{
#if defined(__APPLE__)
  return {&*begin, (std::size_t)std::distance(begin, end)};
#else
  return {begin, end};
#endif
}

void
FLARM::SendEscaped(Port &port, std::span<const std::byte> src,
                   OperationEnvironment &env,
                   std::chrono::steady_clock::duration _timeout)
{
  assert(!src.empty());

  const TimeoutClock timeout(_timeout);

  // Send data byte-by-byte including escaping
  auto p = src.begin();
  const auto end = src.end();
  while (true) {
    const auto special = FindSpecial(p, end);

    if (special != p) {
      /* bulk write of "harmless" characters */

      port.FullWrite(MakeSpan(p, special), env,
                     timeout.GetRemainingOrZero());

      p = special;
    }

    if (p == end)
      break;

    // Check for bytes that need to be escaped and send
    // the appropriate replacements
    if (*p == START_FRAME) {
      port.Write(ESCAPE);
      port.Write(ESCAPE_START);
    } else if (*p == ESCAPE) {
      port.Write(ESCAPE);
      port.Write(ESCAPE_ESCAPE);
    } else
      // Otherwise just send the original byte
      port.Write(*p);

    p++;
  }
}

static std::byte *
ReceiveSomeUnescape(Port &port, std::span<std::byte> dest,
                    OperationEnvironment &env, const TimeoutClock timeout)
{
  /* read "length" bytes from the port, optimistically assuming that
     there are no escaped bytes */

  size_t nbytes = port.WaitAndRead(dest, env, timeout);

  /* unescape in-place */

  std::byte *p = dest.data();
  std::byte *end = dest.data() + nbytes;
  for (const std::byte *src = dest.data(); src != end;) {
    if (*src == FLARM::ESCAPE) {
      ++src;

      std::byte ch;
      if (src == end) {
        /* at the end of the buffer; need to read one more byte */
        port.WaitRead(env, timeout.GetRemainingOrZero());

        ch = (std::byte)port.ReadByte();
      } else
        ch = *src++;

      if (ch == FLARM::ESCAPE_START)
        *p++ = FLARM::START_FRAME;
      else if (ch == FLARM::ESCAPE_ESCAPE)
        *p++ = FLARM::ESCAPE;
      else
        /* unknown escape */
        return nullptr;
    } else
      /* "harmless" byte */
      *p++ = *src++;
  }

  /* return the current end position of the destination buffer; if
     there were escaped bytes, then this function must be called again
     to account for the escaping overhead */
  return p;
}

bool
FLARM::ReceiveEscaped(Port &port, std::span<std::byte> dest,
                      OperationEnvironment &env,
                      std::chrono::steady_clock::duration _timeout)
{
  assert(!dest.empty());

  const TimeoutClock timeout(_timeout);

  // Receive data byte-by-byte including escaping until buffer is full
  std::byte *p = dest.data(), *end = p + dest.size();
  while (p < end) {
    p = ReceiveSomeUnescape(port, {p, std::size_t(end - p)},
                            env, timeout);
    if (p == nullptr)
      return false;
  }

  return true;
}

void
FlarmDevice::SendStartByte()
{
  port.Write(FLARM::START_FRAME);
}

inline void
FlarmDevice::WaitForStartByte(OperationEnvironment &env,
                              std::chrono::steady_clock::duration timeout)
{
  port.WaitForByte(FLARM::START_FRAME, env, timeout);
}

FLARM::FrameHeader
FLARM::PrepareFrameHeader(unsigned sequence_number, MessageType message_type,
                          std::span<const std::byte> payload) noexcept
{
  FrameHeader header;
  header.length = 8 + payload.size();
  header.version = 0;
  header.sequence_number = sequence_number++;
  header.type = message_type;
  header.crc = CalculateCRC(header, payload);
  return header;
}

FLARM::FrameHeader
FlarmDevice::PrepareFrameHeader(FLARM::MessageType message_type,
                                std::span<const std::byte> payload) noexcept
{
  return FLARM::PrepareFrameHeader(sequence_number++, message_type,
                                   payload);
}

void
FlarmDevice::SendFrameHeader(const FLARM::FrameHeader &header,
                             OperationEnvironment &env,
                             std::chrono::steady_clock::duration timeout)
{
  SendEscaped(ReferenceAsBytes(header), env, timeout);
}

bool
FlarmDevice::ReceiveFrameHeader(FLARM::FrameHeader &header,
                                OperationEnvironment &env,
                                std::chrono::steady_clock::duration timeout)
{
  return ReceiveEscaped(ReferenceAsWritableBytes(header),
                        env, timeout);
}

FLARM::MessageType
FlarmDevice::WaitForACKOrNACK(uint16_t sequence_number,
                              AllocatedArray<std::byte> &data, uint16_t &length,
                              OperationEnvironment &env,
                              std::chrono::steady_clock::duration _timeout)
{
  const TimeoutClock timeout(_timeout);

  // Receive frames until timeout or expected frame found
  while (!timeout.HasExpired()) {
    // Wait until the next start byte comes around
    WaitForStartByte(env, timeout.GetRemainingOrZero());

    // Read the following FrameHeader
    FLARM::FrameHeader header;
    if (!ReceiveFrameHeader(header, env, timeout.GetRemainingOrZero()))
      continue;

    // Read and check length of the FrameHeader
    length = header.length;
    if (length <= sizeof(header))
      continue;

    // Calculate payload length
    length -= sizeof(header);

    // Read payload and check length
    data.GrowDiscard(length);
    if (!ReceiveEscaped({data.data(), length},
                        env, timeout.GetRemainingOrZero()))
      continue;

    // Verify CRC
    if (header.crc != FLARM::CalculateCRC(header, {data.data(), length}))
      continue;

    // Check message type
    if (header.type != FLARM::MessageType::ACK &&
        header.type != FLARM::MessageType::NACK)
      continue;

    // Check payload length
    if (length < 2)
      continue;

    // Check whether the received ACK is for the right sequence number
    if (FromLE16(*((const uint16_t *)(const void *)data.data())) ==
        sequence_number)
      return (FLARM::MessageType)header.type;
  }

  return FLARM::MessageType::ERROR;
}

FLARM::MessageType
FlarmDevice::WaitForACKOrNACK(uint16_t sequence_number,
                              OperationEnvironment &env,
                              std::chrono::steady_clock::duration timeout)
{
  AllocatedArray<std::byte> data;
  uint16_t length;
  return WaitForACKOrNACK(sequence_number, data, length, env, timeout);
}

bool
FlarmDevice::WaitForACK(uint16_t sequence_number,
                        OperationEnvironment &env,
                        std::chrono::steady_clock::duration timeout)
{
  return WaitForACKOrNACK(sequence_number, env, timeout) == FLARM::MessageType::ACK;
}

bool
FlarmDevice::BinaryPing(OperationEnvironment &env,
                        std::chrono::steady_clock::duration _timeout)
try {
  const TimeoutClock timeout(_timeout);

  // Create header for sending a binary ping request
  FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MessageType::PING);

  // Send request and wait for positive answer

  SendStartByte();
  SendFrameHeader(header, env, timeout.GetRemainingOrZero());
  return WaitForACK(header.sequence_number, env, timeout.GetRemainingOrZero());
} catch (const DeviceTimeout &) {
  return false;
}

void
FlarmDevice::BinaryReset(OperationEnvironment &env,
                         std::chrono::steady_clock::duration _timeout)
{
  TimeoutClock timeout(_timeout);

  // Create header for sending a binary reset request
  FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MessageType::EXIT);

  // Send request and wait for positive answer
  SendStartByte();
  SendFrameHeader(header, env, timeout.GetRemainingOrZero());
}
