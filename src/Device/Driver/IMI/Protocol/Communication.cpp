/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Communication.hpp"
#include "Protocol.hpp"
#include "Checksum.hpp"
#include "MessageParser.hpp"
#include "Device/Error.hpp"
#include "Device/Port/Port.hpp"
#include "time/TimeoutClock.hpp"
#include "util/ByteOrder.hxx"
#include "util/CRC.hpp"

#include <stdexcept>

#include <string.h>

namespace IMI
{
  extern IMIWORD _serialNumber;
}

void
IMI::Send(Port &port, OperationEnvironment &env,
          IMIBYTE msgID, const void *payload, IMIWORD payloadSize,
          IMIBYTE parameter1, IMIWORD parameter2, IMIWORD parameter3)
{
  Sync sync;
  sync.syncChar1 = IMICOMM_SYNC_CHAR1;
  sync.syncChar2 = IMICOMM_SYNC_CHAR2;
  port.FullWrite(&sync, sizeof(sync), env, std::chrono::seconds{1});

  Header header;
  header.sn = _serialNumber;
  header.msgID = msgID;
  header.parameter1 = parameter1;
  header.parameter2 = parameter2;
  header.parameter3 = parameter3;
  header.payloadSize = payloadSize;

  IMIWORD crc = 0xffff;
  crc = UpdateCRC16CCITT(&header, sizeof(header), crc);

  port.FullWrite(&header, sizeof(header), env, std::chrono::seconds{1});

  if (payloadSize > 0) {
    port.FullWrite(payload, payloadSize, env, std::chrono::seconds{2});
    crc = UpdateCRC16CCITT(payload, payloadSize, crc);
  }

  crc = ToBE16(crc);
  port.FullWrite(&crc, sizeof(crc), env, std::chrono::seconds{1});
}

static constexpr std::chrono::steady_clock::duration
CalcPayloadTimeout(std::size_t payload_size, unsigned baud_rate) noexcept
{
  if (baud_rate == 0)
    /* fallback for timeout calculation */
    baud_rate = 9600;

  return std::chrono::milliseconds(10000 * (payload_size + sizeof(IMI::IMICOMM_MSG_HEADER_SIZE) + 10) / baud_rate);
}

IMI::TMsg
IMI::Receive(Port &port, OperationEnvironment &env,
             std::chrono::steady_clock::duration extra_timeout,
             unsigned expectedPayloadSize)
{
  if (expectedPayloadSize > COMM_MAX_PAYLOAD_SIZE)
    expectedPayloadSize = COMM_MAX_PAYLOAD_SIZE;

  const auto payload_timeout =
    CalcPayloadTimeout(expectedPayloadSize, port.GetBaudrate());

  const TimeoutClock timeout(extra_timeout + payload_timeout);

  // wait for the message
  MessageParser mp;
  while (true) {
    // read message
    IMIBYTE buffer[64];
    size_t bytesRead = port.WaitAndRead(buffer, sizeof(buffer), env, timeout);

    // parse message
    if (auto msg = mp.Parse(buffer, bytesRead))
      // message received
      return *msg;
  }
}

IMI::TMsg
IMI::SendRet(Port &port, OperationEnvironment &env,
             IMIBYTE msgID, const void *payload,
             IMIWORD payloadSize, IMIBYTE reMsgID, IMIWORD retPayloadSize,
             IMIBYTE parameter1, IMIWORD parameter2, IMIWORD parameter3,
             std::chrono::steady_clock::duration extra_timeout,
             int retry)
{
  extra_timeout += CalcPayloadTimeout(payloadSize, port.GetBaudrate());

  while (true) {
    Send(port, env, msgID, payload, payloadSize, parameter1, parameter2,
         parameter3);

    try {
      if (auto msg = Receive(port, env, extra_timeout, retPayloadSize);
          msg.msgID == reMsgID &&
          (retPayloadSize == (IMIWORD)-1 || msg.payloadSize == retPayloadSize))
        return msg;
    } catch (const DeviceTimeout &) {
      if (retry-- == 0)
        throw;
    }
  }
}

static bool
RLEDecompress(IMI::IMIBYTE* dest, const IMI::IMIBYTE *src, unsigned size,
              unsigned destSize)
{
  if (size > destSize)
    return false;

  if (size == destSize) {
    memcpy(dest, src, size);
    return true;
  }

  while (size--) {
    IMI::IMIBYTE b = *src++;

    if (destSize-- == 0)
      return false;

    *dest++ = b;

    if (b == 0 || b == 0xFF) {
      unsigned count = *src++;

      if (size-- == 0)
        return false;

      while (count--) {
        if (destSize-- == 0)
          return false;

        *dest++ = b;
      }
    }
  }

  return destSize == 0;
}

bool
IMI::FlashRead(Port &port, void *buffer, unsigned address, unsigned size,
               OperationEnvironment &env)
{
  if (size == 0)
    return true;

  const auto msg = SendRet(port, env,
                           MSG_FLASH, 0, 0, MSG_FLASH, -1,
                           IMICOMM_BIGPARAM1(address),
                           IMICOMM_BIGPARAM2(address),
                           size, std::chrono::seconds{3}, 2);

  if (size != msg.parameter3)
    throw std::runtime_error("Wrong FLASH result size");

  if (!RLEDecompress((IMIBYTE*)buffer, msg.payload, msg.payloadSize, size))
    throw std::runtime_error("RLE decompression error");

  return true;
}
