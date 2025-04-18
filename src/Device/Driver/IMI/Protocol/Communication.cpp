// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Communication.hpp"
#include "Protocol.hpp"
#include "Checksum.hpp"
#include "MessageParser.hpp"
#include "Device/Error.hpp"
#include "Device/Port/Port.hpp"
#include "time/TimeoutClock.hpp"
#include "util/ByteOrder.hxx"
#include "util/CRC16CCITT.hpp"
#include "util/SpanCast.hxx"

#include <stdexcept>

#include <string.h>

namespace IMI
{
  extern IMIWORD _serialNumber;
}

void
IMI::Send(Port &port, OperationEnvironment &env,
          IMIBYTE msgID, std::span<const std::byte> payload,
          IMIBYTE parameter1, IMIWORD parameter2, IMIWORD parameter3)
{
  Sync sync;
  sync.syncChar1 = IMICOMM_SYNC_CHAR1;
  sync.syncChar2 = IMICOMM_SYNC_CHAR2;
  port.FullWrite(ReferenceAsBytes(sync), env, std::chrono::seconds{1});

  Header header;
  header.sn = _serialNumber;
  header.msgID = msgID;
  header.parameter1 = parameter1;
  header.parameter2 = parameter2;
  header.parameter3 = parameter3;
  header.payloadSize = payload.size();

  IMIWORD crc = 0xffff;
  crc = UpdateCRC16CCITT(ReferenceAsBytes(header), crc);

  port.FullWrite(ReferenceAsBytes(header), env, std::chrono::seconds{1});

  if (!payload.empty()) {
    port.FullWrite(payload, env, std::chrono::seconds{2});
    crc = UpdateCRC16CCITT(payload, crc);
  }

  crc = ToBE16(crc);
  port.FullWrite(ReferenceAsBytes(crc), env,
                 std::chrono::seconds{1});
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
    size_t bytesRead = port.WaitAndRead(std::as_writable_bytes(std::span{buffer}),
                                        env, timeout);

    // parse message
    if (auto msg = mp.Parse(buffer, bytesRead))
      // message received
      return *msg;
  }
}

IMI::TMsg
IMI::SendRet(Port &port, OperationEnvironment &env,
             IMIBYTE msgID, std::span<const std::byte> payload,
             IMIBYTE reMsgID, IMIWORD retPayloadSize,
             IMIBYTE parameter1, IMIWORD parameter2, IMIWORD parameter3,
             std::chrono::steady_clock::duration extra_timeout,
             int retry)
{
  extra_timeout += CalcPayloadTimeout(payload.size(), port.GetBaudrate());

  while (true) {
    Send(port, env, msgID, payload, parameter1, parameter2,
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
                           MSG_FLASH, {}, MSG_FLASH, -1,
                           IMICOMM_BIGPARAM1(address),
                           IMICOMM_BIGPARAM2(address),
                           size, std::chrono::seconds{3}, 2);

  if (size != msg.parameter3)
    throw std::runtime_error("Wrong FLASH result size");

  if (!RLEDecompress((IMIBYTE*)buffer, msg.payload, msg.payloadSize, size))
    throw std::runtime_error("RLE decompression error");

  return true;
}
