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

#ifdef HAVE_POSIX
#include "IO/Async/GlobalIOThread.hpp"
#endif

#include <assert.h>

TCPPort::~TCPPort()
{
  SocketPort::Close();

#ifdef HAVE_POSIX
  if (listener.IsDefined())
    io_thread->LockRemove(listener.Get());
#else
  if (thread.IsDefined()) {
    thread.BeginStop();
    thread.Join();
  }
#endif
}

bool
TCPPort::Open(unsigned port)
{
  if (!listener.CreateTCPListener(port, 1))
    return false;

  /* register the socket in then IOThread or the SocketThread */
#ifdef HAVE_POSIX
  io_thread->LockAdd(listener.Get(), Poll::READ, *this);
#else
  thread.Start();
#endif
  return true;
}

PortState
TCPPort::GetState() const
{
  return listener.IsDefined()
    ? PortState::READY
    : PortState::FAILED;
}

bool
TCPPort::OnFileEvent(int fd, unsigned mask)
{
  assert(listener.IsDefined());

  if (fd == listener.Get()) {
    /* connection should never be defined here */
    assert(SocketPort::GetState() == PortState::FAILED);

    SocketDescriptor s = listener.Accept();
    if (!s.IsDefined())
      return true;

#ifndef HAVE_POSIX
    /* reset the flag so we can wait for it atomically after
       SocketPort::Set() */
    closed_trigger.Reset();
#endif

    SocketPort::Set(std::move(s));

#ifdef HAVE_POSIX
    /* disable the listener socket while the connection socket is
       active */
    return false;
#else
    /* for until the connection SocketThread finishes the connection;
       meanwhile, incoming connections are ignored */
    closed_trigger.Wait();
    SocketPort::Close();

    /* now continue listening for incoming connections */
    return true;
#endif
  } else {
    /* this event affects the connection socket */

    if (!SocketPort::OnFileEvent(fd, mask)) {
      /* the connection was closed; continue listening on incoming
         connections */

#ifdef HAVE_POSIX
      /* close the connection, unregister the event, and reinstate the
         listener socket */
      SocketPort::Close();
      io_thread->Add(listener.Get(), Poll::READ, *this);
#else
      /* we must not call SocketPort::Close() here because it may
         deadlock, waiting forever for this thread to finish; instead,
         wake up the listener thread, and let it handle the event */
      closed_trigger.Signal();
#endif
      return false;
    } else
      /* continue reading from the connection */
      return true;
  }
}
