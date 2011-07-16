/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Device/Driver/LX/Protocol.hpp"
#include "Operation.hpp"

bool
LX::CommandMode(Port &port)
{
  /* switch to command mode, first attempt */
  port.Write(SYN);

  /* now flush all of the remaining input */
  port.SetRxTimeout(10);
  port.FullFlush(20);

  /* the port is clean now; try the SYN/ACK procedure up to three
     times */
  return port.SetRxTimeout(500) &&
    (Connect(port) || Connect(port) || Connect(port)) &&
    /* ... and configure the timeout */
    port.SetRxTimeout(5000);
}

void
LX::CommandModeQuick(Port &port, OperationEnvironment &env)
{
  port.Write(SYN);
  env.Sleep(500);
  port.Write(SYN);
  env.Sleep(500);
  port.Write(SYN);
  env.Sleep(500);
}

bool
LX::SendPacket(Port &port, enum command command,
               const void *data, size_t length,
               unsigned timeout_ms)
{
  return SendCommand(port, command) &&
    port.FullWrite(data, length, timeout_ms) &&
    port.Write(calc_crc(data, length, 0xff));
}

bool
LX::ReceivePacket(Port &port, enum command command,
                  void *data, size_t length,
                  unsigned timeout_ms)
{
  port.Flush();
  return SendCommand(port, command) &&
    ReadCRC(port, data, length, timeout_ms);
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
LX::ReadCRC(Port &port, void *buffer, size_t length, unsigned timeout_ms)
{
  uint8_t crc;

  return port.FullRead(buffer, length, timeout_ms) &&
    port.FullRead(&crc, sizeof(crc), timeout_ms) &&
    calc_crc(buffer, length, 0xff) == crc;
}
