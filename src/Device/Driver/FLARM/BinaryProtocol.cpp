// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "CRC16.hpp"
#include "Device/Error.hpp"
#include "Device/Port/Port.hpp"
#include "LogFile.hpp"
#include "time/TimeoutClock.hpp"
#include "util/SpanCast.hxx"

#include <algorithm> // for std::find_if()
#include <vector>

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

static void
AppendEscaped(std::vector<std::byte> &dest,
              std::span<const std::byte> src) noexcept
{
  for (const std::byte b : src) {
    if (b == FLARM::START_FRAME) {
      dest.push_back(FLARM::ESCAPE);
      dest.push_back(FLARM::ESCAPE_START);
    } else if (b == FLARM::ESCAPE) {
      dest.push_back(FLARM::ESCAPE);
      dest.push_back(FLARM::ESCAPE_ESCAPE);
    } else
      dest.push_back(b);
  }
}

void
FLARM::SendFrame(Port &port, const FrameHeader &header,
                 std::span<const std::byte> payload,
                 OperationEnvironment &env,
                 std::chrono::steady_clock::duration timeout)
{
  std::vector<std::byte> frame;
  frame.reserve(1 + 2 * (sizeof(header) + payload.size()));

  frame.push_back(START_FRAME);
  AppendEscaped(frame, ReferenceAsBytes(header));
  AppendEscaped(frame, payload);

  port.FullWrite(frame, env, timeout);
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

/**
 * Give up on a frame after this much silence, even if the caller
 * allows more time for the whole frame.  A bridge which drops the
 * rest of a frame (e.g. a Bluetooth LE adapter with a short transmit
 * queue) is detected in seconds instead of blocking until the frame
 * timeout has expired, and the caller can retry that much earlier.
 * Any link which is still delivering data keeps the frame alive,
 * because each chunk restarts this timeout.
 */
static constexpr std::chrono::steady_clock::duration
FRAME_IDLE_TIMEOUT = std::chrono::seconds{3};

bool
FLARM::ReceiveEscaped(Port &port, std::span<std::byte> dest,
                      OperationEnvironment &env,
                      std::chrono::steady_clock::duration _timeout)
{
  assert(!dest.empty());

  const TimeoutClock timeout(_timeout);

  // Receive data byte-by-byte including escaping until buffer is full
  std::byte *p = dest.data(), *end = p + dest.size();
  try {
    while (p < end) {
      const TimeoutClock idle_timeout{std::min(timeout.GetRemainingOrZero(),
                                               FRAME_IDLE_TIMEOUT)};

      p = ReceiveSomeUnescape(port, {p, std::size_t(end - p)},
                              env, idle_timeout);
      if (p == nullptr)
        return false;
    }
  } catch (const DeviceTimeout &) {
#ifndef NDEBUG
    if (p > dest.data())
      /* the frame stopped arriving in the middle; over a Bluetooth
         LE bridge, this typically means its buffer overflowed and
         the rest of the frame was dropped */
      LogFormat("FLARM: timeout after receiving %u of %u frame bytes",
                unsigned(p - dest.data()), unsigned(dest.size()));
#endif
    throw;
  }

  return true;
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

  lost_frame_length = 0;

  // Receive frames until timeout or expected frame found
  while (!timeout.HasExpired()) {
    // Wait until the next start byte comes around
    WaitForStartByte(env, timeout.GetRemainingOrZero());

    // Read the following FrameHeader
    FLARM::FrameHeader header;
    if (!ReceiveFrameHeader(header, env, timeout.GetRemainingOrZero())) {
#ifndef NDEBUG
      LogFormat("FLARM: malformed frame header");
#endif
      continue;
    }

    // Read and check length of the FrameHeader
    length = header.length;
    if (length <= sizeof(header)) {
#ifndef NDEBUG
      LogFormat("FLARM: discarding short frame (type=0x%02x length=%u)",
                unsigned(header.type), unsigned(length));
#endif
      continue;
    }

    // Calculate payload length
    length -= sizeof(header);

    // Read payload and check length
    data.GrowDiscard(length);
    try {
      if (!ReceiveEscaped({data.data(), length},
                          env, timeout.GetRemainingOrZero())) {
#ifndef NDEBUG
        LogFormat("FLARM: malformed frame payload (type=0x%02x length=%u)",
                  unsigned(header.type), unsigned(length));
#endif
        continue;
      }
    } catch (const DeviceTimeout &) {
      /* remember how much this frame would have carried: a restarted
         flight download can step over it if that part of the file has
         already been saved */
      lost_frame_length = length;
      throw;
    }

    // Verify CRC
    if (header.crc != FLARM::CalculateCRC(header, {data.data(), length})) {
#ifndef NDEBUG
      LogFormat("FLARM: discarding frame with bad CRC (type=0x%02x length=%u)",
                unsigned(header.type), unsigned(length));
#endif
      continue;
    }

    // Check message type
    if (header.type != FLARM::MessageType::ACK &&
        header.type != FLARM::MessageType::NACK) {
#ifndef NDEBUG
      LogFormat("FLARM: ignoring frame (type=0x%02x length=%u)",
                unsigned(header.type), unsigned(length));
#endif
      continue;
    }

    // Check payload length
    if (length < 2) {
#ifndef NDEBUG
      LogFormat("FLARM: discarding %s without sequence number",
                header.type == FLARM::MessageType::ACK ? "ACK" : "NACK");
#endif
      continue;
    }

    // Check whether the received ACK is for the right sequence number
    const uint16_t received_sequence_number =
      FromLE16(*((const uint16_t *)(const void *)data.data()));
    if (received_sequence_number == sequence_number)
      return (FLARM::MessageType)header.type;

#ifndef NDEBUG
    LogFormat("FLARM: ignoring %s with sequence %u (expecting %u)",
              header.type == FLARM::MessageType::ACK ? "ACK" : "NACK",
              unsigned(received_sequence_number), unsigned(sequence_number));
#endif
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

  SendFrame(header, {}, env, timeout.GetRemainingOrZero());
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
  SendFrame(header, {}, env, timeout.GetRemainingOrZero());
}
