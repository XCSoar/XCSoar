/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#include "Communication.hpp"
#include "Protocol.hpp"
#include "Checksum.hpp"
#include "Device/Declaration.hpp"
#include "Device/Driver.hpp"
#include "MessageParser.hpp"
#include "Device/Port.hpp"
#include "OS/Clock.hpp"

namespace IMI
{
  extern IMIWORD _serialNumber;
}

bool
IMI::Send(Port &port, const TMsg &msg)
{
  return port.FullWrite(&msg, IMICOMM_MSG_HEADER_SIZE + msg.payloadSize + 2, 2000);
}

bool
IMI::Send(Port &port, IMIBYTE msgID, const void *payload, IMIWORD payloadSize,
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

  return Send(port, msg);
}

const IMI::TMsg *
IMI::Receive(Port &port, unsigned extraTimeout, unsigned expectedPayloadSize)
{
  if (expectedPayloadSize > COMM_MAX_PAYLOAD_SIZE)
    expectedPayloadSize = COMM_MAX_PAYLOAD_SIZE;

  // set timeout
  unsigned baudrate = port.GetBaudrate();
  if (baudrate == 0)
    return NULL;

  unsigned timeout = extraTimeout + 10000 * (expectedPayloadSize
      + sizeof(IMICOMM_MSG_HEADER_SIZE) + 10) / baudrate;
  if (!port.SetRxTimeout(timeout))
    return NULL;

  // wait for the message
  const TMsg *msg = NULL;
  timeout += MonotonicClockMS();
  while (MonotonicClockMS() < timeout) {
    // read message
    IMIBYTE buffer[64];
    int bytesRead = port.Read(buffer, sizeof(buffer));
    if (bytesRead == 0)
      continue;
    if (bytesRead == -1)
      break;

    // parse message
    const TMsg *lastMsg = MessageParser::Parse(buffer, bytesRead);
    if (lastMsg) {
      // message received
      if (lastMsg->msgID == MSG_ACK_NOTCONFIG)
        Disconnect(port);
      else if (lastMsg->msgID != MSG_CFG_KEEPCONFIG)
        msg = lastMsg;

      break;
    }
  }

  // restore timeout
  if (!port.SetRxTimeout(0))
    return NULL;

  return msg;
}

const IMI::TMsg *
IMI::SendRet(Port &port, IMIBYTE msgID, const void *payload,
             IMIWORD payloadSize, IMIBYTE reMsgID, IMIWORD retPayloadSize,
             IMIBYTE parameter1, IMIWORD parameter2, IMIWORD parameter3,
             unsigned extraTimeout, int retry)
{
  unsigned baudRate = port.GetBaudrate();
  if (baudRate == 0)
    return NULL;

  extraTimeout += 10000 * (payloadSize + sizeof(IMICOMM_MSG_HEADER_SIZE) + 10)
      / baudRate;
  while (retry--) {
    if (Send(port, msgID, payload, payloadSize, parameter1, parameter2,
             parameter3)) {
      const TMsg *msg = Receive(port, extraTimeout, retPayloadSize);
      if (msg && msg->msgID == reMsgID && (retPayloadSize == (IMIWORD)-1
          || msg->payloadSize == retPayloadSize))
        return msg;
    }
  }

  return NULL;
}
