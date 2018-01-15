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

#include "DumpPort.hpp"
#include "HexDump.hpp"
#include "OS/Clock.hpp"

#include <stdint.h>
#include <stdio.h>

#ifdef __clang__
/* true, the nullptr cast below is a bad kludge */
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

DumpPort::DumpPort(Port *_port)
  :Port(nullptr, *(DataHandler *)nullptr), port(_port),
   until_ms(-1) {}

DumpPort::~DumpPort()
{
  delete port;
}

void
DumpPort::EnableTemporarily(unsigned duration_ms)
{
  until_ms = MonotonicClockMS() + duration_ms;
}

bool
DumpPort::CheckEnabled()
{
  if (until_ms == 0)
    return false;

  if (until_ms == unsigned(-1))
    return true;

  if (MonotonicClockMS() >= until_ms) {
    /* duration has just expired; clear to avoid calling
       MonotonicClockMS() again in the next call */
    until_ms = 0;
    return false;
  }

  return true;
}

PortState
DumpPort::GetState() const
{
  return port->GetState();
}

bool
DumpPort::WaitConnected(OperationEnvironment &env)
{
  return port->WaitConnected(env);
}

size_t
DumpPort::Write(const void *data, size_t length)
{
  const bool enabled = CheckEnabled();
  if (enabled)
    LogFormat("Write(%u)", (unsigned)length);

  size_t nbytes = port->Write(data, length);

  if (enabled) {
    LogFormat("Write(%u)=%u", (unsigned)length, (unsigned)nbytes);
    HexDump("W ", data, nbytes);
  }

  return nbytes;
}

bool
DumpPort::Drain()
{
  if (CheckEnabled())
    LogFormat("Drain");

  return port->Drain();
}

void
DumpPort::Flush()
{
  if (CheckEnabled())
    LogFormat("Flush");

  port->Flush();
}

unsigned
DumpPort::GetBaudrate() const
{
  return port->GetBaudrate();
}

bool
DumpPort::SetBaudrate(unsigned baud_rate)
{
  if (CheckEnabled())
    LogFormat("SetBaudrate %u", baud_rate);

  return port->SetBaudrate(baud_rate);
}

bool
DumpPort::StopRxThread()
{
  if (CheckEnabled())
    LogFormat("StopRxThread");

  return port->StopRxThread();
}

bool
DumpPort::StartRxThread()
{
  if (CheckEnabled())
    LogFormat("StartRxThread");

  return port->StartRxThread();
}

int
DumpPort::Read(void *buffer, size_t size)
{
  const bool enabled = CheckEnabled();
  if (enabled)
    LogFormat("Read(%u)", (unsigned)size);

  int nbytes = port->Read(buffer, size);

  if (enabled) {
    LogFormat("Read(%u)=%d", (unsigned)size, nbytes);
    if (nbytes > 0)
      HexDump("R ", buffer, nbytes);
  }

  return nbytes;
}

Port::WaitResult
DumpPort::WaitRead(unsigned timeout_ms)
{
  const bool enabled = CheckEnabled();
  if (enabled)
    LogFormat("WaitRead %u", timeout_ms);

  Port::WaitResult result = port->WaitRead(timeout_ms);

  if (enabled)
    LogFormat("WaitRead %u = %d", timeout_ms, (int)result);

  return result;
}
