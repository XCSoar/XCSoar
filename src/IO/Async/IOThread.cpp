/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "IOThread.hpp"
#include "FileEventHandler.hpp"

bool
IOThread::Start()
{
  assert(!IsDefined());
  assert(loop.IsEmpty());

  quit = false;

  if (!pipe.Create())
    return false;

  loop.Add(pipe.GetReadFD(), READ, *this);

  return Thread::Start();
}

void
IOThread::Stop()
{
  /* set the "quit" flag and wake up the thread */
  loop.Lock();
  quit = true;
  loop.Unlock();
  pipe.Signal();

  /* wait for the thread to finish */
  Join();

  loop.Remove(pipe.GetReadFD());
}

void
IOThread::LockAdd(FileDescriptor fd, unsigned mask, FileEventHandler &handler)
{
  loop.Lock();
  const bool old_modified = loop.IsModified();
  Add(fd, mask, handler);
  const bool new_modified = loop.IsModified();
  loop.Unlock();

  if (!old_modified && new_modified)
    pipe.Signal();
}

void
IOThread::LockRemove(FileDescriptor fd)
{
  loop.Lock();
  const bool old_modified = loop.IsModified();
  Remove(fd);
  const bool new_modified = loop.IsModified();

  if (new_modified && !IsInside()) {
    /* this method is synchronous: after returning, all handlers must
       be finished */

    loop.WaitUntilNotRunning();
  }

  loop.Unlock();

  if (!old_modified && new_modified)
    pipe.Signal();
}

void
IOThread::Run()
{
  loop.Lock();

  while (!quit) {
    loop.Wait();

    if (quit)
      break;

    loop.Dispatch();
  }

  loop.Unlock();
}

bool
IOThread::OnFileEvent(FileDescriptor fd, unsigned mask)
{
  assert(fd == pipe.GetReadFD());

  pipe.Read();
  return true;
}

