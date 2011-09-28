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

#include "Protocol.hpp"
#include "CRC16.hpp"
#include "Device/Port/Port.hpp"
#include "Operation.hpp"
#include "PeriodClock.hpp"

#include <string.h>

bool
Volkslogger::Reset(Port &port, OperationEnvironment &env, unsigned n)
{
  static const unsigned delay = 2;

  while (n-- > 0) {
    port.Write(CAN);
    env.Sleep(delay);
  }

  return true;
}

bool
Volkslogger::Handshake(Port &port, OperationEnvironment &env,
                       unsigned timeout_ms)
{
  PeriodClock clock;
  clock.update();

  do { // Solange R's aussenden, bis ein L zurückkommt
    port.Write('R');
    env.Sleep(30);

    if (clock.check(timeout_ms))
      return false;
  } while (port.GetChar() != 'L');

  unsigned count = 1;
  while (true) { // Auf 4 hintereinanderfolgende L's warten
    int ch = port.GetChar();
    if (ch >= 0) {
      if (ch != 'L')
        return false;

      count++;
      if (count >= 4)
        return true;
    }

    if (clock.check(timeout_ms))
      return false;
  }
}

bool
Volkslogger::Connect(Port &port, OperationEnvironment &env,
                     unsigned timeout_ms)
{
  return Reset(port, env, 10) && Handshake(port, env, timeout_ms);
}

static bool
SendWithCRC(Port &port, const void *data, size_t length)
{
  if (!port.FullWrite(data, length, 2000))
    return false;

  uint16_t crc16 = UpdateCRC(data, length, 0);
  return port.Write(crc16 >> 8) && port.Write(crc16 & 0xff);
}

bool
Volkslogger::SendCommand(Port &port, OperationEnvironment &env,
                         Command cmd, uint8_t param1, uint8_t param2)
{
  static const unsigned delay = 2;

  /* flush buffers */
  port.SetRxTimeout(20);
  port.FullFlush(100);
  port.SetRxTimeout(500);

  /* reset command interpreter */
  if (!Reset(port, env, 6))
    return false;

  /* send command packet */

  const uint8_t cmdarray[8] = {
    (uint8_t)cmd, param1, param2,
    0, 0, 0, 0, 0,
  };

  port.Write(ENQ);
  env.Sleep(delay);

  if (!SendWithCRC(port, cmdarray, sizeof(cmdarray)))
    return false;

  /* wait for confirmation */
  const unsigned timeout_ms = 4000;
  PeriodClock clock;
  clock.update();

  int c;
  while ((c = port.GetChar()) < 0) {
    if (clock.check(timeout_ms))
      return false;
  }

  return c == 0;
}

gcc_const
static int
GetBaudRateIndex(unsigned baud_rate)
{
  switch(baud_rate) {
  case 9600:
    return 1;

  case 19200:
    return 2;

  case 38400:
    return 3;

  case 57600:
    return 4;

  case 115200:
    return 5;

  default:
    return -1;
  }
}

bool
Volkslogger::SendCommandSwitchBaudRate(Port &port, OperationEnvironment &env,
                                       Command cmd, unsigned baud_rate)
{
  int baud_rate_index = GetBaudRateIndex(baud_rate);
  if (baud_rate_index < 0)
    return false;

  if (!SendCommand(port, env, cmd, 0, baud_rate_index))
    return false;

  port.SetBaudrate(baud_rate);
  return true;
}

bool
Volkslogger::WaitForACK(Port &port, OperationEnvironment &env)
{
  const unsigned timeout_ms = 30000;

  PeriodClock clock;
  clock.update();

  int ch;
  while (true) {
    ch = port.GetChar();
    if (ch == ACK)
      return true;
    else if (ch < 0 || env.IsCancelled() || clock.check(timeout_ms))
      return false;
  }
}

int
Volkslogger::ReadBulk(Port &port, OperationEnvironment &env,
                      void *buffer, unsigned max_length)
{
  unsigned nbytes = 0;
  bool dle_r = false;
  uint16_t crc16 = 0;
  bool start = false, ende = false;

  memset(buffer, 0xff, max_length);

  uint8_t *p = (uint8_t *)buffer;
  env.Sleep(300);
  while (!ende) {
    // Zeichen anfordern und darauf warten
    port.Write(ACK);

    int ch = port.GetChar();
    if (ch < 0)
      return -1;

    // dabei ist Benutzerabbruch jederzeit möglich
    if (env.IsCancelled()) {
      env.Sleep(10);
      port.Write(CAN);
      port.Write(CAN);
      port.Write(CAN);
      return -1;
    }

    // oder aber das empfangene Zeichen wird ausgewertet
    switch (ch) {
    case DLE:
      if (!dle_r) {             //!DLE, DLE -> Achtung!
        dle_r = true;
      }
      else { 	                 // DLE, DLE -> DLE-Zeichen
        dle_r = false;
        if (start) {
          if(nbytes < max_length)
            *p++ = ch;
          nbytes++;
          crc16 = UpdateCRC(ch, crc16);
        }
      }
      break;
    case ETX:
      if (!dle_r) {             //!DLE, ETX -> Zeichen
        if (start) {
          if(nbytes < max_length) {
            *p++ = ch;
          }
          nbytes++;
          crc16 = UpdateCRC(ch, crc16);
        };
      }
      else {
        if (start) {
          ende = true; // DLE, ETX -> Blockende
          dle_r = false;
        }
      }
      break;
    case STX:
      if (!dle_r) { //!DLE, STX -> Zeichen
        if (start) {
          if(nbytes < max_length)
            *p++ = ch;
          nbytes++;
          crc16 = UpdateCRC(ch, crc16);
        }
      }
      else {
        start = true; // DLE, STX -> Blockstart
        dle_r = false;
        crc16 = 0;
      }
      break;
    default:
      if (start) {
        if(nbytes < max_length)
          *p++ = ch;
        nbytes++;
        crc16 = UpdateCRC(ch, crc16);
      }
      break;
    }
  }

  env.Sleep(100);

  if (crc16 != 0)
    return -1;

  if (nbytes < 2)
    return 0;

  // CRC am Ende abschneiden
  return nbytes - 2;
}

bool
Volkslogger::WriteBulk(Port &port, OperationEnvironment &env,
                       const void *buffer, unsigned length)
{
  const unsigned delay = 1;

  env.SetProgressRange(length);

  uint16_t crc16 = 0;
  const uint8_t *p = (const uint8_t *)buffer, *end = p + length;
  while (p < end) {
    unsigned n = end - p;
    if (n > 400)
      n = 400;

    n = port.Write(p, n);
    if (n == 0)
      return false;

    crc16 = UpdateCRC(p, n, crc16);
    p += n;

    env.SetProgressPosition(p - (const uint8_t *)buffer);

    /* throttle sending a bit, or the Volkslogger's receive buffer
       will overrun */
    env.Sleep(delay * 100);
  }

  port.Write(crc16 >> 8);
  env.Sleep(delay);
  port.Write(crc16 & 0xff);
  env.Sleep(delay);

  return true;
}

int
Volkslogger::SendCommandReadBulk(Port &port, OperationEnvironment &env,
                                 Command cmd,
                                 void *buffer, unsigned max_length)
{
  return SendCommand(port, env, cmd)
    ? ReadBulk(port, env, buffer, max_length)
    : -1;
}

int
Volkslogger::SendCommandReadBulk(Port &port, OperationEnvironment &env,
                                 Command cmd,
                                 void *buffer, unsigned max_length,
                                 unsigned baud_rate)
{
  int baud_rate_index = GetBaudRateIndex(baud_rate);
  if (baud_rate_index < 0)
    return -1;

  if (!SendCommand(port, env, cmd, 0, baud_rate_index))
    return -1;

  unsigned old_baud_rate = port.SetBaudrate(baud_rate);
  int nbytes = ReadBulk(port, env, buffer, max_length);

  port.SetBaudrate(old_baud_rate);

  return nbytes;
}
