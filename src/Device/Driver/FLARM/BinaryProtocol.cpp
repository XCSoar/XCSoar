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

#include "Device.hpp"
#include "CRC16.hpp"
#include "Device/Port/Port.hpp"
#include "Time/TimeoutClock.hpp"

gcc_pure
static const uint8_t *
FindSpecial(const uint8_t *const begin, const uint8_t *const end)
{
  const uint8_t *start = std::find(begin, end, FLARM::START_FRAME);
  const uint8_t *escape = std::find(begin, end, FLARM::ESCAPE);
  return std::min(start, escape);
}

bool
FLARM::SendEscaped(Port &port, const void *buffer, size_t length,
                   OperationEnvironment &env, unsigned timeout_ms)
{
  assert(buffer != nullptr);
  assert(length > 0);

  const TimeoutClock timeout(timeout_ms);

  // Send data byte-by-byte including escaping
  const uint8_t *p = (const uint8_t *)buffer, *end = p + length;
  while (true) {
    const uint8_t *special = FindSpecial(p, end);

    if (special > p) {
      /* bulk write of "harmless" characters */

      if (!port.FullWrite(p, special - p, env, timeout.GetRemainingOrZero()))
        return false;

      p = special;
    }

    if (p == end)
      break;

    // Check for bytes that need to be escaped and send
    // the appropriate replacements
    bool result;
    if (*p == START_FRAME)
      result = port.Write(ESCAPE) && port.Write(ESCAPE_START);
    else if (*p == ESCAPE)
      result = port.Write(ESCAPE) && port.Write(ESCAPE_ESCAPE);
    else
      // Otherwise just send the original byte
      result = port.Write(*p);

    if (!result)
      return false;

    p++;
  }

  return true;
}

static uint8_t *
ReceiveSomeUnescape(Port &port, uint8_t *buffer, size_t length,
                    OperationEnvironment &env, const TimeoutClock timeout)
{
  /* read "length" bytes from the port, optimistically assuming that
     there are no escaped bytes */

  size_t nbytes = port.WaitAndRead(buffer, length, env, timeout);
  if (nbytes == 0)
    return nullptr;

  /* unescape in-place */

  uint8_t *end = buffer + nbytes;
  for (const uint8_t *src = buffer; src != end;) {
    if (*src == FLARM::ESCAPE) {
      ++src;

      int ch;
      if (src == end) {
        /* at the end of the buffer; need to read one more byte */
        if (port.WaitRead(env, timeout.GetRemainingOrZero()) != Port::WaitResult::READY)
          return nullptr;

        ch = port.GetChar();
      } else
        ch = *src++;

      if (ch == FLARM::ESCAPE_START)
        *buffer++ = FLARM::START_FRAME;
      else if (ch == FLARM::ESCAPE_ESCAPE)
        *buffer++ = FLARM::ESCAPE;
      else
        /* unknown escape */
        return nullptr;
    } else
      /* "harmless" byte */
      *buffer++ = *src++;
  }

  /* return the current end position of the destination buffer; if
     there were escaped bytes, then this function must be called again
     to account for the escaping overhead */
  return buffer;
}

bool
FLARM::ReceiveEscaped(Port &port, void *buffer, size_t length,
                      OperationEnvironment &env, unsigned timeout_ms)
{
  assert(buffer != nullptr);
  assert(length > 0);

  const TimeoutClock timeout(timeout_ms);

  // Receive data byte-by-byte including escaping until buffer is full
  uint8_t *p = (uint8_t *)buffer, *end = p + length;
  while (p < end) {
    p = ReceiveSomeUnescape(port, p, end - p, env, timeout);
    if (p == nullptr)
      return false;
  }

  return true;
}

bool
FlarmDevice::SendStartByte()
{
  return port.Write(FLARM::START_FRAME);
}

bool
FlarmDevice::WaitForStartByte(OperationEnvironment &env, unsigned timeout_ms)
{
  return port.WaitForChar(FLARM::START_FRAME, env, timeout_ms) == Port::WaitResult::READY;
}

FLARM::FrameHeader
FLARM::PrepareFrameHeader(unsigned sequence_number, MessageType message_type,
                          const void *data, size_t length)
{
  assert((data != nullptr && length > 0) ||
         (data == nullptr && length == 0));

  FrameHeader header;
  header.length = 8 + length;
  header.version = 0;
  header.sequence_number = sequence_number++;
  header.type = (uint8_t)message_type;
  header.crc = CalculateCRC(header, data, length);
  return header;
}

FLARM::FrameHeader
FlarmDevice::PrepareFrameHeader(FLARM::MessageType message_type,
                                const void *data, size_t length)
{
  return FLARM::PrepareFrameHeader(sequence_number++, message_type,
                                   data, length);
}

bool
FlarmDevice::SendFrameHeader(const FLARM::FrameHeader &header,
                             OperationEnvironment &env, unsigned timeout_ms)
{
  return SendEscaped(&header, sizeof(header), env, timeout_ms);
}

bool
FlarmDevice::ReceiveFrameHeader(FLARM::FrameHeader &header,
                                OperationEnvironment &env, unsigned timeout_ms)
{
  return ReceiveEscaped(&header, sizeof(header), env, timeout_ms);
}

FLARM::MessageType
FlarmDevice::WaitForACKOrNACK(uint16_t sequence_number,
                              AllocatedArray<uint8_t> &data, uint16_t &length,
                              OperationEnvironment &env, unsigned timeout_ms)
{
  const TimeoutClock timeout(timeout_ms);

  // Receive frames until timeout or expected frame found
  while (!timeout.HasExpired()) {
    // Wait until the next start byte comes around
    if (!WaitForStartByte(env, timeout.GetRemainingOrZero()))
      continue;

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
    if (!ReceiveEscaped(data.begin(), length,
                        env, timeout.GetRemainingOrZero()))
      continue;

    // Verify CRC
    if (header.crc != FLARM::CalculateCRC(header, data.begin(), length))
      continue;

    // Check message type
    if (header.type != FLARM::MT_ACK && header.type != FLARM::MT_NACK)
      continue;

    // Check payload length
    if (length < 2)
      continue;

    // Check whether the received ACK is for the right sequence number
    if (FromLE16(*((const uint16_t *)(const void *)data.begin())) ==
        sequence_number)
      return (FLARM::MessageType)header.type;
  }

  return FLARM::MT_ERROR;
}

FLARM::MessageType
FlarmDevice::WaitForACKOrNACK(uint16_t sequence_number,
                              OperationEnvironment &env, unsigned timeout_ms)
{
  AllocatedArray<uint8_t> data;
  uint16_t length;
  return WaitForACKOrNACK(sequence_number, data, length, env, timeout_ms);
}

bool
FlarmDevice::WaitForACK(uint16_t sequence_number,
                        OperationEnvironment &env, unsigned timeout_ms)
{
  return WaitForACKOrNACK(sequence_number, env, timeout_ms) == FLARM::MT_ACK;
}

bool
FlarmDevice::BinaryPing(OperationEnvironment &env, unsigned timeout_ms)
{
  const TimeoutClock timeout(timeout_ms);

  // Create header for sending a binary ping request
  FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MT_PING);

  // Send request and wait for positive answer
  return SendStartByte() &&
    SendFrameHeader(header, env, timeout.GetRemainingOrZero()) &&
    WaitForACK(header.sequence_number, env, timeout.GetRemainingOrZero());
}

bool
FlarmDevice::BinaryReset(OperationEnvironment &env, unsigned timeout_ms)
{
  TimeoutClock timeout(timeout_ms);

  // Create header for sending a binary reset request
  FLARM::FrameHeader header = PrepareFrameHeader(FLARM::MT_EXIT);

  // Send request and wait for positive answer
  return SendStartByte() &&
    SendFrameHeader(header, env, timeout.GetRemainingOrZero());
}
