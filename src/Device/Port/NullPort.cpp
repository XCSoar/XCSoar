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

#include "NullPort.hpp"

#include <stdio.h>

NullPort::NullPort()
  :Port(nullptr, *(DataHandler *)this)
{
}

NullPort::NullPort(DataHandler &_handler)
  :Port(nullptr, _handler)
{
}

PortState
NullPort::GetState() const
{
  return PortState::READY;
}

bool
NullPort::Drain()
{
  return true;
}

void
NullPort::Flush()
{
}

size_t
NullPort::Write(const void *data, size_t length)
{
  return length;
}

bool
NullPort::StopRxThread()
{
  return true;
}

bool
NullPort::StartRxThread()
{
  return true;
}

unsigned
NullPort::GetBaudrate() const
{
  return 0;
}

bool
NullPort::SetBaudrate(unsigned baud_rate)
{
  return true;
}

int
NullPort::Read(void *Buffer, size_t Size)
{
  return -1;
}

Port::WaitResult
NullPort::WaitRead(unsigned timeout_ms)
{
  return WaitResult::FAILED;
}

void
NullPort::DataReceived(const void *data, size_t length)
{
}
