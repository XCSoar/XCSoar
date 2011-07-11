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

#include "Device/DumpPort.hpp"
#include "HexDump.hpp"

#include <stdint.h>
#include <stdio.h>

size_t
DumpPort::Write(const void *data, size_t length)
{
  LogStartUp(_T("Write(%u)"), (unsigned)length);
  size_t nbytes = other.Write(data, length);
  LogStartUp(_T("Write(%u)=%u"), (unsigned)length, (unsigned)nbytes);
  HexDump(_T("W "), data, nbytes);
  return nbytes;
}

void
DumpPort::Flush()
{
  LogStartUp(_T("Flush"));
  other.Flush();
}

bool
DumpPort::SetRxTimeout(unsigned timeout_ms)
{
  LogStartUp(_T("SetRxTimeout %u"), timeout_ms);
  return other.SetRxTimeout(timeout_ms);
}

unsigned
DumpPort::GetBaudrate() const
{
  return other.GetBaudrate();
}

unsigned
DumpPort::SetBaudrate(unsigned baud_rate)
{
  LogStartUp(_T("SetBaudrate %u"), baud_rate);
  return other.SetBaudrate(baud_rate);
}

bool
DumpPort::StopRxThread()
{
  LogStartUp(_T("StopRxThread"));
  return other.StopRxThread();
}

bool
DumpPort::StartRxThread()
{
  LogStartUp(_T("StartRxThread"));
  return other.StartRxThread();
}

int
DumpPort::Read(void *buffer, size_t size)
{
  LogStartUp(_T("Read(%u)"), (unsigned)size);
  int nbytes = other.Read(buffer, size);
  LogStartUp(_T("Read(%u)=%d"), (unsigned)size, nbytes);
  if (nbytes > 0)
    HexDump(_T("R "), buffer, nbytes);
  return nbytes;
}

