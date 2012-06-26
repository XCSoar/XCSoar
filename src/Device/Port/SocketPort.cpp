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

#include "SocketPort.hpp"

#include <assert.h>

SocketPort::~SocketPort()
{
  StopRxThread();
}

void
SocketPort::Set(SocketDescriptor &&_socket)
{
  assert(!socket.IsDefined());
  assert(_socket.IsDefined());

  socket = std::move(_socket);
}

bool
SocketPort::OpenUDPListener(unsigned port)
{
  return socket.CreateUDPListener(port);
}

bool
SocketPort::IsValid() const
{
  return socket.IsDefined();
}

bool
SocketPort::Drain()
{
  /* writes are synchronous */
  return true;
}

void
SocketPort::Flush()
{
}

bool
SocketPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::IsInside());

  // Make sure the port is still open
  if (!IsValid())
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::IsDefined())
    return true;

  BeginStop();

  Thread::Join();

  return true;
}

size_t
SocketPort::Write(const void *data, size_t length)
{
  if (!socket.IsDefined())
    return 0;

  ssize_t nbytes = socket.Write((const char *)data, length);
  return nbytes < 0 ? 0 : nbytes;
}

bool
SocketPort::StartRxThread()
{
  if (Thread::IsDefined())
    /* already running */
    return true;

  // Make sure the port was opened correctly
  if (!IsValid())
    return false;

  // Start the receive thread
  StoppableThread::Start();
  return true;
}


unsigned
SocketPort::GetBaudrate() const
{
  return 0;
}


bool
SocketPort::SetBaudrate(unsigned baud_rate)
{
  return true;
}

int
SocketPort::Read(void *buffer, size_t length)
{
  if (!socket.IsDefined())
    return -1;

  if (socket.WaitReadable(0) <= 0)
    return -1;

  return socket.Read(buffer, length);
}

Port::WaitResult
SocketPort::WaitRead(unsigned timeout_ms)
{
  if (!socket.IsDefined())
    return WaitResult::FAILED;

  int ret = socket.WaitReadable(timeout_ms);
  if (ret > 0)
    return WaitResult::READY;
  else if (ret == 0)
    return WaitResult::TIMEOUT;
  else
    return WaitResult::FAILED;
}

void
SocketPort::Run()
{
  char buffer[1024];

  while (!CheckStopped()) {
    assert(socket.IsDefined());

    int ret = socket.WaitReadable(250);
    if (ret > 0) {
      ssize_t nbytes = socket.Read(buffer, sizeof(buffer));
      if (nbytes <= 0) {
        break;
      }
      handler.DataReceived(buffer, nbytes);
    } else if (ret < 0) {
      break;
    }
  }
}

