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
#include "Device/Port/Port.hpp"
#include "TimeoutClock.hpp"
#include "Operation/Operation.hpp"

#define FLARM_STARTFRAME 0x73
#define FLARM_ESCAPE 0x78
#define FLARM_ESC_ESC 0x55
#define FLARM_ESC_START 0x31

bool
FlarmDevice::SendEscaped(const void *buffer, size_t length,
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
    if (*p == FLARM_STARTFRAME)
      result = port.Write(FLARM_ESCAPE) && port.Write(FLARM_ESC_START);
    else if (*p == FLARM_ESCAPE)
      result = port.Write(FLARM_ESCAPE) && port.Write(FLARM_ESC_ESC);
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
FlarmDevice::ReceiveEscaped(void *buffer, size_t length,
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
    if (c == FLARM_STARTFRAME)
      return false;

    if (c == FLARM_ESCAPE) {
      // Read next character after the ESCAPE
      if (port.WaitRead(env, timeout.GetRemainingOrZero()) != Port::WaitResult::READY)
        return false;

      int c2 = port.GetChar();
      if (c2 == -1 || timeout.HasExpired())
        return false;

      switch (c2) {
      case FLARM_ESC_ESC:
        // Nothing to do because ESCAPE is already in the buffer
        break;

      case FLARM_ESC_START:
        // Replace the byte in the buffer with the unescaped STARTFRAME byte
        c = FLARM_STARTFRAME;
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
  return port.Write(FLARM_STARTFRAME);
}

bool
FlarmDevice::WaitForStartByte(OperationEnvironment &env, unsigned timeout_ms)
{
  return port.WaitForChar(FLARM_STARTFRAME, env, timeout_ms) == Port::WaitResult::READY;
}

/*
 * Source: FLARM_BINCOMM.pdf
 */
static uint16_t
crc_update(uint16_t crc, uint8_t data)
{
  crc = crc ^ ((uint16_t)data << 8);
  for (unsigned i = 0; i < 8; ++i) {
    if (crc & 0x8000)
      crc = (crc << 1) ^ 0x1021;
    else
      crc <<= 1;
  }
  return crc;
}

uint16_t
FlarmDevice::CalculateCRC(const FrameHeader &header,
                          const void *data, size_t length)
{
  assert((data != NULL && length > 0) || (data == NULL && length == 0));

  uint16_t crc = 0x00;

  const uint8_t *_header = (const uint8_t *)&header;
  for (unsigned i = 0; i < 6; ++i, ++_header)
    crc = crc_update(crc, *_header);

  if (length == 0 || data == NULL)
    return crc;

  const uint8_t *_data = (const uint8_t *)data;
  for (unsigned i = 0; i < length; ++i, ++_data)
    crc = crc_update(crc, *_data);

  return crc;
}

FlarmDevice::FrameHeader
FlarmDevice::PrepareFrameHeader(MessageType message_type,
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

bool
FlarmDevice::SendFrameHeader(const FrameHeader &header,
                             OperationEnvironment &env, unsigned timeout_ms)
{
  return SendEscaped(&header, sizeof(header), env, timeout_ms);
}

bool
FlarmDevice::ReceiveFrameHeader(FrameHeader &header,
                                OperationEnvironment &env, unsigned timeout_ms)
{
  return ReceiveEscaped(&header, sizeof(header), env, timeout_ms);
}

FlarmDevice::MessageType
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
    FrameHeader header;
    if (!ReceiveFrameHeader(header, env, timeout.GetRemainingOrZero()))
      continue;

    // Read and check length of the FrameHeader
    length = header.GetLenght();
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
    if (header.GetCRC() != CalculateCRC(header, data.begin(), length))
      continue;

    // Check message type
    if (header.type != MT_ACK && header.type != MT_NACK)
      continue;

    // Check payload length
    if (length < 2)
      continue;

    // Check whether the received ACK is for the right sequence number
    if (FromLE16(*((const uint16_t *)(const void *)data.begin())) ==
        sequence_number)
      return (MessageType)header.type;
  }

  return MT_ERROR;
}

FlarmDevice::MessageType
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
  return WaitForACKOrNACK(sequence_number, env, timeout_ms) == MT_ACK;
}

bool
FlarmDevice::BinaryPing(OperationEnvironment &env, unsigned timeout_ms)
{
  const TimeoutClock timeout(timeout_ms);

  // Create header for sending a binary ping request
  FrameHeader header = PrepareFrameHeader(MT_PING);

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
  FrameHeader header = PrepareFrameHeader(MT_EXIT);

  // Send request and wait for positive answer
  return SendStartByte() &&
    SendFrameHeader(header, env, timeout.GetRemainingOrZero());
}

bool
FlarmDevice::BinaryMode(OperationEnvironment &env)
{
  if (mode == Mode::BINARY)
    return true;

  port.StopRxThread();

  // "Binary mode is engaged by sending the text command "$PFLAX"
  // (including a newline character) to Flarm."
  if (!Send("PFLAX", env))
    return false;

  // Remember that we should now be in binary mode (for further assert() calls)
  mode = Mode::BINARY;

  // "After switching, connection should again be checked by issuing a ping."
  // Testing has revealed that switching the protocol takes a certain amount
  // of time (around 1.5 sec). Due to that it is recommended to issue new pings
  // for a certain time until the ping is ACKed properly or a timeout occurs.
  for (unsigned i = 0; i < 10; ++i) {
    if (env.IsCancelled()) {
      BinaryReset(env, 200);
      mode = Mode::UNKNOWN;
      return false;
    }

    if (BinaryPing(env, 500))
      // We are now in binary mode and have verified that with a binary ping
      return true;
  }

  // Apparently the switch to binary mode didn't work
  mode = Mode::UNKNOWN;
  return false;
}
