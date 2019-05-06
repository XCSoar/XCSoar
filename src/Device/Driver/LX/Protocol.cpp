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

#include "Protocol.hpp"
#include "Operation/Operation.hpp"

#include <assert.h>

bool
LX::CommandMode(Port &port, OperationEnvironment &env)
{
  /* switch to command mode, first attempt */

  if (!SendSYN(port) ||
      !port.FullFlush(env, std::chrono::milliseconds(50),
                      std::chrono::milliseconds(200)))
    return false;

  /* the port is clean now; try the SYN/ACK procedure up to three
     times */
  for (unsigned i = 0; i < 100 && !env.IsCancelled(); ++i)
    if (Connect(port, env))
      /* make sure all remaining ACKs are flushed */
      return port.FullFlush(env, std::chrono::milliseconds(200),
                            std::chrono::milliseconds(500));

  return false;
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

bool
LX::SendPacket(Port &port, Command command,
               const void *data, size_t length,
               OperationEnvironment &env,
               std::chrono::steady_clock::duration timeout)
{
  return SendCommand(port, command) &&
    port.FullWrite(data, length, env, timeout) &&
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
  return SendCommand(port, command) &&
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

    if (!CommandMode(port, env))
      return false;

    port.Flush();
  }
}

uint8_t
LX::calc_crc_char(uint8_t d, uint8_t crc)
{
  uint8_t tmp;
  const uint8_t crcpoly = 0x69;
  int count;

  for (count = 8; --count >= 0; d <<= 1) {
    tmp = crc ^ d;
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

  return port.FullRead(buffer, length, env,
                       first_timeout, subsequent_timeout,
                       total_timeout) &&
    port.FullRead(&crc, sizeof(crc), env,
                  subsequent_timeout, subsequent_timeout,
                  subsequent_timeout) &&
    calc_crc(buffer, length, 0xff) == crc;
}
