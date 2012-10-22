/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include <stdint.h>
#include <stdio.h>

DumpPort::DumpPort(Port *_port):Port(*(DataHandler *)NULL), port(_port) {}

DumpPort::~DumpPort()
{
  delete port;
}

PortState
DumpPort::GetState() const
{
  return port->GetState();
}

size_t
DumpPort::Write(const void *data, size_t length)
{
  LogFormat("Write(%u)", (unsigned)length);
  size_t nbytes = port->Write(data, length);
  LogFormat("Write(%u)=%u", (unsigned)length, (unsigned)nbytes);
  HexDump(_T("W "), data, nbytes);
  return nbytes;
}

bool
DumpPort::Drain()
{
  LogFormat("Drain");
  return port->Drain();
}

void
DumpPort::Flush()
{
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
  LogFormat("SetBaudrate %u", baud_rate);
  return port->SetBaudrate(baud_rate);
}

bool
DumpPort::StopRxThread()
{
  LogFormat("StopRxThread");
  return port->StopRxThread();
}

bool
DumpPort::StartRxThread()
{
  LogFormat("StartRxThread");
  return port->StartRxThread();
}

int
DumpPort::Read(void *buffer, size_t size)
{
  LogFormat("Read(%u)", (unsigned)size);
  int nbytes = port->Read(buffer, size);
  LogFormat("Read(%u)=%d", (unsigned)size, nbytes);
  if (nbytes > 0)
    HexDump(_T("R "), buffer, nbytes);
  return nbytes;
}

Port::WaitResult
DumpPort::WaitRead(unsigned timeout_ms)
{
  LogFormat("WaitRead %u", timeout_ms);
  Port::WaitResult result = port->WaitRead(timeout_ms);
  LogFormat("WaitRead %u = %d", timeout_ms, (int)result);
  return result;
}
