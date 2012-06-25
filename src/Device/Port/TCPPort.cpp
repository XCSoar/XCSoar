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

void
TCPPort::Run()
{
  while (!CheckStopped()) {
    assert(listener.IsDefined());
    /* connection should never be defined here */
    assert(!SocketPort::IsValid());

    int ret = listener.WaitReadable(250);
    if (ret > 0) {
      SocketDescriptor s = listener.Accept();
      if (s.IsDefined()) {
        SocketPort::Set(std::move(s));
        /* reads from existing client connection, SocketPort::Run()
           returns whenever the current connection fails, so it can be
           closed also on this side */
        SocketPort::Run();
        SocketPort::Close();
      }
    } else if (ret < 0) {
      listener.Close();
      break;
    }
  }
}



