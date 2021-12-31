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

#include "Protocol.hpp"
#include "util/CRC.hpp"
#include "Device/Error.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"
#include "time/TimeoutClock.hpp"

#include <string.h>

void
Volkslogger::Reset(Port &port, OperationEnvironment &env, unsigned n)
{
  static constexpr auto delay = std::chrono::milliseconds(2);

  while (n-- > 0) {
    port.Write(CAN);
    env.Sleep(delay);
  }
}

void
Volkslogger::Handshake(Port &port, OperationEnvironment &env,
                       std::chrono::steady_clock::duration _timeout)
{
  TimeoutClock timeout(_timeout);

  while (true) { // Solange R's aussenden, bis ein L zurückkommt
    port.Write('R');

    auto remaining = timeout.GetRemainingSigned();
    if (remaining.count() < 0)
      throw DeviceTimeout{"Handshake timeout"};

    if (remaining > std::chrono::milliseconds(500))
      remaining = std::chrono::milliseconds(500);

    try {
      port.WaitForChar('L', env, remaining);
      break;
    } catch (const DeviceTimeout &) {
      /* timeout, try again */
    }
  }

  unsigned count = 1;
  while (true) { // Auf 4 hintereinanderfolgende L's warten
    const auto remaining = timeout.GetRemainingSigned();
    if (remaining.count() < 0)
      throw DeviceTimeout{"Handshake timeout"};

    port.WaitForChar('L', env, remaining);

    count++;
    if (count >= 4)
      return;
  }
}

void
Volkslogger::Connect(Port &port, OperationEnvironment &env,
                     std::chrono::steady_clock::duration timeout)
{
  Reset(port, env, 10);
  Handshake(port, env, timeout);
}

void
Volkslogger::ConnectAndFlush(Port &port, OperationEnvironment &env,
                             std::chrono::steady_clock::duration timeout)
{
  port.Flush();

  Connect(port, env, timeout);
  port.FullFlush(env, std::chrono::milliseconds(50),
                 std::chrono::milliseconds(300));
}

static void
SendWithCRC(Port &port, const void *data, size_t length,
            OperationEnvironment &env)
{
  port.FullWrite(data, length, env, std::chrono::seconds(2));

  uint16_t crc16 = UpdateCRC16CCITT(data, length, 0);
  port.Write(crc16 >> 8);
  port.Write(crc16 & 0xff);
}

bool
Volkslogger::SendCommand(Port &port, OperationEnvironment &env,
                         Command cmd, uint8_t param1, uint8_t param2)
{
  static constexpr auto delay = std::chrono::milliseconds(2);

  /* flush buffers */
  port.FullFlush(env, std::chrono::milliseconds(20),
                 std::chrono::milliseconds(100));

  /* reset command interpreter */
  Reset(port, env, 6);

  /* send command packet */

  const uint8_t cmdarray[8] = {
    (uint8_t)cmd, param1, param2,
    0, 0, 0, 0, 0,
  };

  port.Write(ENQ);

  env.Sleep(delay);

  SendWithCRC(port, cmdarray, sizeof(cmdarray), env);

  /* wait for confirmation */

  port.WaitRead(env, std::chrono::seconds(4));
  return port.ReadByte() == std::byte{0};
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
                                       Command cmd, uint8_t param1,
                                       unsigned baud_rate)
{
  int baud_rate_index = GetBaudRateIndex(baud_rate);
  if (baud_rate_index < 0)
    return false;

  if (!SendCommand(port, env, cmd, param1, baud_rate_index))
    return false;

  port.SetBaudrate(baud_rate);
  return true;
}

void
Volkslogger::WaitForACK(Port &port, OperationEnvironment &env)
{
  port.WaitForChar(ACK, env, std::chrono::seconds(30));
}

int
Volkslogger::ReadBulk(Port &port, OperationEnvironment &env,
                      void *buffer, size_t max_length,
                      std::chrono::steady_clock::duration timeout_firstchar)
{
  unsigned nbytes = 0;
  bool dle_r = false;
  uint16_t crc16 = 0;
  bool start = false, ende = false;

  memset(buffer, 0xff, max_length);

  uint8_t *p = (uint8_t *)buffer;

  static constexpr auto TIMEOUT_NORMAL = std::chrono::seconds(2);
  /**
   * We need to wait longer for the first char to
   * give the logger time to calculate security
   * when downloading a log-file.
   * Therefore timeout_firstchar is configurable.
   * If the timeout parameter is not specified or 0,
   * set standard timeout
   */
  if (timeout_firstchar == std::chrono::steady_clock::duration::zero())
    timeout_firstchar = TIMEOUT_NORMAL;

  while (!ende) {
    // Zeichen anfordern und darauf warten

    port.Write(ACK);

    // Set longer timeout on first char
    const std::chrono::steady_clock::duration timeout = start ? TIMEOUT_NORMAL : timeout_firstchar;
    port.WaitRead(env, timeout);

    const auto ch = (uint8_t)port.ReadByte();

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
          crc16 = UpdateCRC16CCITT(ch, crc16);
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
          crc16 = UpdateCRC16CCITT(ch, crc16);
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
          crc16 = UpdateCRC16CCITT(ch, crc16);
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
        crc16 = UpdateCRC16CCITT(ch, crc16);
      }
      break;
    }
  }

  env.Sleep(std::chrono::milliseconds(100));

  if (crc16 != 0)
    return -1;

  if (nbytes < 2)
    return 0;

  // CRC am Ende abschneiden
  return nbytes - 2;
}

void
Volkslogger::WriteBulk(Port &port, OperationEnvironment &env,
                       const void *buffer, unsigned length)
{
  static constexpr auto delay = std::chrono::milliseconds(100);

  env.SetProgressRange(length);

  uint16_t crc16 = 0;
  const uint8_t *p = (const uint8_t *)buffer, *end = p + length;
  while (p < end) {
    unsigned n = end - p;
    if (n > 400)
      n = 400;

    n = port.Write(p, n);

    crc16 = UpdateCRC16CCITT(p, n, crc16);
    p += n;

    env.SetProgressPosition(p - (const uint8_t *)buffer);

    /* throttle sending a bit, or the Volkslogger's receive buffer
       will overrun */
    env.Sleep(delay);
  }

  port.Write(crc16 >> 8);
  port.Write(crc16 & 0xff);
}

int
Volkslogger::SendCommandReadBulk(Port &port, OperationEnvironment &env,
                                 Command cmd,
                                 void *buffer, size_t max_length,
                                 std::chrono::steady_clock::duration timeout_firstchar)
{
  return SendCommand(port, env, cmd)
    ? ReadBulk(port, env, buffer, max_length, timeout_firstchar)
    : -1;
}

int
Volkslogger::SendCommandReadBulk(Port &port, unsigned baud_rate,
                                 OperationEnvironment &env,
                                 Command cmd, uint8_t param1,
                                 void *buffer, size_t max_length,
                                 std::chrono::steady_clock::duration timeout_firstchar)
{
  unsigned old_baud_rate = port.GetBaudrate();

  if (old_baud_rate != 0) {
    if (!SendCommandSwitchBaudRate(port, env, cmd, param1, baud_rate))
      return -1;

    /* after switching baud rates, this sleep time is necessary; it has
       been verified experimentally */
    env.Sleep(std::chrono::milliseconds(300));
  } else {
    /* port does not support baud rate switching, use plain
       SendCommand() without new baud rate */

    if (!SendCommand(port, env, cmd, param1))
      return -1;
  }

  int nbytes = ReadBulk(port, env, buffer, max_length, timeout_firstchar);

  if (old_baud_rate != 0)
    port.SetBaudrate(old_baud_rate);

  return nbytes;
}

bool
Volkslogger::SendCommandWriteBulk(Port &port, OperationEnvironment &env,
                                  Command cmd,
                                  const void *data, size_t size)
{
  if (!SendCommand(port, env, cmd, 0, 0))
    return false;

  WaitForACK(port, env);

  env.Sleep(std::chrono::milliseconds(100));

  WriteBulk(port, env, data, size);
  WaitForACK(port, env);
  return true;
}

size_t
Volkslogger::ReadFlight(Port &port, unsigned databaud,
                        OperationEnvironment &env,
                        unsigned flightnr, bool secmode,
                        void *buffer, size_t buffersize)
{
  const Volkslogger::Command cmd = secmode
    ? Volkslogger::cmd_GFS
    : Volkslogger::cmd_GFL;

  /*
   * It is necessary to wait long for the first reply from
   * the Logger in ReadBulk.
   * Since the VL needs time to calculate the Security of
   * the log before it responds.
   */
  static constexpr auto timeout_firstchar = std::chrono::minutes(10);

  // Download binary log data supports BulkBaudrate
  int groesse = SendCommandReadBulk(port, databaud, env, cmd,
                                    flightnr, buffer, buffersize,
                                    timeout_firstchar);
  if (groesse <= 0)
    return 0;

  // read signature
  env.Sleep(std::chrono::milliseconds(300));

  /*
   * Testing has shown that downloading the Signature does not support
   * BulkRate. It has to be done with standard IO Rate (9600)
   */
  int sgr = SendCommandReadBulk(port, env, Volkslogger::cmd_SIG,
                                (uint8_t *)buffer + groesse,
                                buffersize - groesse);
  if (sgr <= 0)
    return 0;

  return groesse + sgr;
}
