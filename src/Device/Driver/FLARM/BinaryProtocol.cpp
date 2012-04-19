/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "TimeoutClock.hpp"
#include "Operation/Operation.hpp"

bool
FLARM::SendEscaped(Port &port, const void *buffer, size_t length,
                   OperationEnvironment &env, unsigned timeout_ms)
{
  assert(buffer != NULL);
  assert(length > 0);

  const TimeoutClock timeout(timeout_ms);

  // Send data byte-by-byte including escaping
  const char *p = (const char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.HasExpired() || env.IsCancelled())
      return false;

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

bool
FLARM::ReceiveEscaped(Port &port, void *buffer, size_t length,
                      OperationEnvironment &env, unsigned timeout_ms)
{
  assert(buffer != NULL);
  assert(length > 0);

  const TimeoutClock timeout(timeout_ms);

  // Receive data byte-by-byte including escaping until buffer is full
  char *p = (char *)buffer, *end = p + length;
  while (p < end) {
    if (timeout.HasExpired())
      return false;

    if (port.WaitRead(env, timeout.GetRemainingOrZero()) != Port::WaitResult::READY)
      return false;

    // Read single character from port
    int c = port.GetChar();
    if (c == -1)
      return false;

    // If a STARTFRAME was detected inside of the frame -> cancel
    if (c == START_FRAME)
      return false;

    if (c == ESCAPE) {
      // Read next character after the ESCAPE
      if (port.WaitRead(env, timeout.GetRemainingOrZero()) != Port::WaitResult::READY)
        return false;

      int c2 = port.GetChar();
      if (c2 == -1 || timeout.HasExpired())
        return false;

      switch (c2) {
      case ESCAPE_ESCAPE:
        // Nothing to do because ESCAPE is already in the buffer
        break;

      case ESCAPE_START:
        // Replace the byte in the buffer with the unescaped STARTFRAME byte
        c = START_FRAME;
        break;

      default:
        // ESCAPE must be followed by either ESCAPE or STARTFRAME -> cancel
        return false;
      }
    }

    *p = c;
    p++;
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
  assert((data != NULL && length > 0) || (data == NULL && length == 0));

  FrameHeader header;
  header.SetLength(8 + length);
  header.version = 0;
  header.SetSequenceNumber(sequence_number++);
  header.type = (uint8_t)message_type;
  header.SetCRC(CalculateCRC(header, data, length));
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
    length = header.GetLength();
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
    if (header.GetCRC() != FLARM::CalculateCRC(header, data.begin(), length))
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
    WaitForACK(header.GetSequenceNumber(), env, timeout.GetRemainingOrZero());
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
