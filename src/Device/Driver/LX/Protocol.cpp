// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Protocol.hpp"
#include "Device/Error.hpp"
#include "Operation/Operation.hpp"

#include <cassert>

void
LX::CommandMode(Port &port, OperationEnvironment &env)
{
  /* switch to command mode, first attempt */

  SendSYN(port);

  port.FullFlush(env, std::chrono::milliseconds(50),
                 std::chrono::milliseconds(200));

  /* the port is clean now; try the SYN/ACK procedure up to three
     times */
  for (unsigned i = 0;; ++i) {
    try {
      Connect(port, env);
    } catch (const DeviceTimeout &) {
      if (i >= 100)
        throw;
      /* retry */
      continue;
    }

    /* make sure all remaining ACKs are flushed */
    port.FullFlush(env, std::chrono::milliseconds(200),
                   std::chrono::milliseconds(500));
    return;
  }
}

void
LX::CommandModeQuick(Port &port, OperationEnvironment &env)
{
  SendSYN(port);
  env.Sleep(std::chrono::milliseconds(500));
  SendSYN(port);
  env.Sleep(std::chrono::milliseconds(500));
  SendSYN(port);
  env.Sleep(std::chrono::milliseconds(500));
}

void
LX::SendPacket(Port &port, Command command,
               const void *data, size_t length,
               OperationEnvironment &env,
               std::chrono::steady_clock::duration timeout)
{
  SendCommand(port, command);

  port.FullWrite(data, length, env, timeout);
  port.Write(calc_crc(data, length, 0xff));
}

bool
LX::ReceivePacket(Port &port, Command command,
                  void *data, size_t length, OperationEnvironment &env,
                  std::chrono::steady_clock::duration first_timeout,
                  std::chrono::steady_clock::duration subsequent_timeout,
                  std::chrono::steady_clock::duration total_timeout)
{
  port.Flush();
  SendCommand(port, command);
  return
    ReadCRC(port, data, length, env,
            first_timeout, subsequent_timeout, total_timeout);
}

bool
LX::ReceivePacketRetry(Port &port, Command command,
                       void *data, size_t length, OperationEnvironment &env,
                       std::chrono::steady_clock::duration first_timeout,
                       std::chrono::steady_clock::duration subsequent_timeout,
                       std::chrono::steady_clock::duration total_timeout,
                       unsigned n_retries)
{
  assert(n_retries > 0);

  while (true) {
    if (ReceivePacket(port, command, data, length, env,
                      first_timeout, subsequent_timeout,
                      total_timeout))
      return true;

    if (n_retries-- == 0)
      return false;

    CommandMode(port, env);

    port.Flush();
  }
}

uint8_t
LX::calc_crc_char(uint8_t d, uint8_t crc)
{
  const uint8_t crcpoly = 0x69;
  int count;

  for (count = 8; --count >= 0; d <<= 1) {
    uint8_t tmp = crc ^ d;
    crc <<= 1;
    if (tmp & 0x80)
      crc ^= crcpoly;
  }
  return crc;
}

uint8_t
LX::calc_crc(const void *p0, size_t len, uint8_t crc)
{
  const uint8_t *p = (const uint8_t *)p0;
  size_t i;

  for (i = 0; i < len; i++)
    crc = calc_crc_char(p[i], crc);

  return crc;
}

bool
LX::ReadCRC(Port &port, void *buffer, size_t length, OperationEnvironment &env,
            std::chrono::steady_clock::duration first_timeout,
            std::chrono::steady_clock::duration subsequent_timeout,
            std::chrono::steady_clock::duration total_timeout)
{
  uint8_t crc;

  port.FullRead(buffer, length, env,
                first_timeout, subsequent_timeout,
                total_timeout);
  port.FullRead(&crc, sizeof(crc), env,
                subsequent_timeout, subsequent_timeout,
                subsequent_timeout);

  return calc_crc(buffer, length, 0xff) == crc;
}
