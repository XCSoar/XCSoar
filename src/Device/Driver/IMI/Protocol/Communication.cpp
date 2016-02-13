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

#include "Communication.hpp"
#include "Protocol.hpp"
#include "Checksum.hpp"
#include "MessageParser.hpp"
#include "Device/Port/Port.hpp"
#include "Time/TimeoutClock.hpp"

#include <string.h>

namespace IMI
{
  extern IMIWORD _serialNumber;
  extern bool _connected;
}

bool
IMI::Send(Port &port, const TMsg &msg, OperationEnvironment &env)
{
  return port.FullWrite(&msg, IMICOMM_MSG_HEADER_SIZE + msg.payloadSize + 2,
                        env, 2000);
}

bool
IMI::Send(Port &port, OperationEnvironment &env,
          IMIBYTE msgID, const void *payload, IMIWORD payloadSize,
          IMIBYTE parameter1, IMIWORD parameter2, IMIWORD parameter3)
{
  if (payloadSize > COMM_MAX_PAYLOAD_SIZE)
    return false;

  TMsg msg;
  memset(&msg, 0, sizeof(msg));

  msg.syncChar1 = IMICOMM_SYNC_CHAR1;
  msg.syncChar2 = IMICOMM_SYNC_CHAR2;
  msg.sn = _serialNumber;
  msg.msgID = msgID;
  msg.parameter1 = parameter1;
  msg.parameter2 = parameter2;
  msg.parameter3 = parameter3;
  msg.payloadSize = payloadSize;
  memcpy(msg.payload, payload, payloadSize);

  IMIWORD crc = CRC16Checksum(((IMIBYTE*)&msg) + 2,
                              payloadSize + IMICOMM_MSG_HEADER_SIZE - 2);
  msg.payload[payloadSize] = (IMIBYTE)(crc >> 8);
  msg.payload[payloadSize + 1] = (IMIBYTE)crc;

  return Send(port, msg, env);
}

const IMI::TMsg *
IMI::Receive(Port &port, OperationEnvironment &env,
             unsigned extraTimeout, unsigned expectedPayloadSize)
{
  if (expectedPayloadSize > COMM_MAX_PAYLOAD_SIZE)
    expectedPayloadSize = COMM_MAX_PAYLOAD_SIZE;

  // set timeout
  unsigned baudrate = port.GetBaudrate();
  if (baudrate == 0)
    /* fallback for timeout calculation */
    baudrate = 9600;

  const TimeoutClock timeout(extraTimeout + 10000 *
                             (expectedPayloadSize + sizeof(IMICOMM_MSG_HEADER_SIZE) + 10) / baudrate);

  // wait for the message
  while (true) {
    // read message
    IMIBYTE buffer[64];
    size_t bytesRead = port.WaitAndRead(buffer, sizeof(buffer), env, timeout);
    if (bytesRead == 0)
      return nullptr;

    // parse message
    const TMsg *msg = MessageParser::Parse(buffer, bytesRead);
    if (msg != nullptr) {
      // message received
      if (msg->msgID == MSG_ACK_NOTCONFIG) {
        Disconnect(port, env);
        return nullptr;
      } else if (msg->msgID == MSG_CFG_KEEPCONFIG)
        return nullptr;
      else
        return msg;
    }
  }
}

const IMI::TMsg *
IMI::SendRet(Port &port, OperationEnvironment &env,
             IMIBYTE msgID, const void *payload,
             IMIWORD payloadSize, IMIBYTE reMsgID, IMIWORD retPayloadSize,
             IMIBYTE parameter1, IMIWORD parameter2, IMIWORD parameter3,
             unsigned extraTimeout, int retry)
{
  unsigned baudRate = port.GetBaudrate();
  if (baudRate == 0)
    /* fallback for timeout calculation */
    baudRate = 9600;

  extraTimeout += 10000 * (payloadSize + sizeof(IMICOMM_MSG_HEADER_SIZE) + 10)
      / baudRate;
  while (retry--) {
    if (Send(port, env, msgID, payload, payloadSize, parameter1, parameter2,
             parameter3)) {
      const TMsg *msg = Receive(port, env, extraTimeout, retPayloadSize);
      if (msg && msg->msgID == reMsgID && (retPayloadSize == (IMIWORD)-1
          || msg->payloadSize == retPayloadSize))
        return msg;
    }
  }

  return nullptr;
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
  if (!_connected)
    return false;

  if (size == 0)
    return true;

  const TMsg *pMsg = SendRet(port, env,
                             MSG_FLASH, 0, 0, MSG_FLASH, -1,
                             IMICOMM_BIGPARAM1(address),
                             IMICOMM_BIGPARAM2(address),
                             size, 300, 2);

  if (pMsg == nullptr || size != pMsg->parameter3)
    return false;

  return RLEDecompress((IMIBYTE*)buffer, pMsg->payload, pMsg->payloadSize, size);
}
