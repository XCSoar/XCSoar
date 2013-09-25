/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "IOLoop.hpp"
#include "FileEventHandler.hpp"

void
IOLoop::Add(int fd, unsigned mask, FileEventHandler &handler)
{
  assert(fd >= 0);
  assert(mask != 0);

  auto i = files.insert(std::make_pair(fd, File(fd, mask, handler)));
  File &file = i.first->second;

  if (!i.second) {
    /* already exists; update it */

    assert(file.mask == 0 || file.handler == &handler);

    if (file.mask == mask)
      /* mask has not been modified, no-op */
      return;

    file.mask = mask;
    file.ready_mask &= mask;
    file.handler = &handler;
  }

  file.modified = true;

  /* schedule an Update() call */
  modified = true;
}

void
IOLoop::Remove(int fd)
{
  assert(fd >= 0);

  auto i = files.find(fd);
  if (i == files.end())
    return;

  assert(i->first == fd);

  File &file = i->second;
  assert(file.fd == fd);

  if (file.mask != 0) {
    /* schedule for removal */
    file.mask = file.ready_mask = 0;
    file.modified = true;
    modified = true;
  }
}

void
IOLoop::Update()
{
  for (auto i = files.begin(), end = files.end(); i != end;) {
    File &file = i->second;
    assert(file.fd == i->first);

    if (!file.modified) {
      ++i;
      continue;
    }

    file.modified = false;

    poll.SetMask(file.fd, file.mask);
    if (file.mask == 0)
      i = files.erase(i);
    else
      ++i;
  }
}

IOLoop::File *
IOLoop::CollectReady()
{
  File *ready = NULL;
  for (auto i = poll.begin(), end = poll.end(); i != end; ++i) {
    const int fd = *i;
    const unsigned mask = i.GetMask();
    assert(mask != 0);

    auto j = files.find(fd);
    assert(j != files.end());

    File &file = j->second;
    assert(file.fd == fd);

    file.ready_mask = mask;
    file.next_ready = ready;
    ready = &file;
  }

  return ready;
}

void
IOLoop::HandleReady(File *ready)
{
  if (ready == NULL)
    return;

  /* set the "running" flag so other threads calling LockRemove() know
     they have to wait for this method to finish */
  assert(!running);
  running = true;

  do {
    /* must check ready_mask, just in case some other handler has
       removed the current file descriptor */
    const unsigned mask = ready->ready_mask;
    if (ready->mask == 0 || mask == 0) {
      ready = ready->next_ready;
      continue;
    }

    const auto handler = ready->handler;

    mutex.Unlock();
    bool result = ready->handler->OnFileEvent(ready->fd, mask);
    mutex.Lock();

    if (!result && ready->mask != 0 && ready->handler == handler) {
      ready->mask = 0;
      ready->modified = true;
      modified = true;
    }

    ready = ready->next_ready;
  } while (ready != NULL);

  /* clear the "running" flag and wake up threads waiting inside
     LockRemove() */
  running = false;
  cond.Broadcast();
}

void
IOLoop::Wait(int timeout_ms)
{
  if (modified) {
    modified = false;
    Update();
  }

  mutex.Unlock();
  poll.Wait(timeout_ms);
  mutex.Lock();
}

void
IOLoop::Dispatch()
{
  HandleReady(CollectReady());
}
