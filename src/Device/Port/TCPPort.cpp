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

#include "TCPPort.hpp"

#include <assert.h>

TCPPort::~TCPPort()
{
  if (listener.IsDefined())
    StopRxThread();
}

bool
TCPPort::Open(unsigned port)
{
  return listener.CreateTCPListener(port, 1);
}

bool
TCPPort::IsValid() const
{
  return listener.IsDefined();
}

bool
TCPPort::Drain()
{
  /* writes are synchronous */
  return true;
}

void
TCPPort::Flush()
{
}

void
TCPPort::Run()
{
  char buffer[1024];

  while (!CheckStopped()) {
    assert(listener.IsDefined());

    if (!connection.IsDefined()) {
      /* accept new connection */

      int ret = listener.WaitReadable(250);
      if (ret > 0)
        connection = listener.Accept();
      else if (ret < 0) {
        listener.Close();
        break;
      }
    } else {
      /* read from existing client connection */

      int ret = connection.WaitReadable(250);
      if (ret > 0) {
        ssize_t nbytes = connection.Read(buffer, sizeof(buffer));
        if (nbytes <= 0) {
          connection.Close();
          continue;
        }

        handler.DataReceived(buffer, nbytes);
      } else if (ret < 0) {
        connection.Close();
      }
    }
  }
}

size_t
TCPPort::Write(const void *data, size_t length)
{
  if (!connection.IsDefined())
    return 0;

  ssize_t nbytes = connection.Write((const char *)data, length);
  return nbytes < 0 ? 0 : nbytes;
}

bool
TCPPort::StopRxThread()
{
  // Make sure the thread isn't terminating itself
  assert(!Thread::IsInside());

  // Make sure the port is still open
  if (!listener.IsDefined())
    return false;

  // If the thread is not running, cancel the rest of the function
  if (!Thread::IsDefined())
    return true;

  BeginStop();

  Thread::Join();

  return true;
}

bool
TCPPort::StartRxThread()
{
  if (Thread::IsDefined())
    /* already running */
    return true;

  // Make sure the port was opened correctly
  if (!listener.IsDefined())
    return false;

  // Start the receive thread
  StoppableThread::Start();
  return true;
}

unsigned
TCPPort::GetBaudrate() const
{
  return 0;
}

bool
TCPPort::SetBaudrate(unsigned baud_rate)
{
  return true;
}

int
TCPPort::Read(void *buffer, size_t length)
{
  if (!connection.IsDefined())
    return -1;

  if (connection.WaitReadable(0) <= 0)
    return -1;

  return connection.Read(buffer, length);
}

Port::WaitResult
TCPPort::WaitRead(unsigned timeout_ms)
{
  if (!connection.IsDefined())
    return WaitResult::FAILED;

  int ret = connection.WaitReadable(timeout_ms);
  if (ret > 0)
    return WaitResult::READY;
  else if (ret == 0)
    return WaitResult::TIMEOUT;
  else
    return WaitResult::FAILED;
}
