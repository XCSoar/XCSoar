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

#include "IOThread.hpp"
#include "FileEventHandler.hpp"

bool
IOThread::Start()
{
  assert(!IsDefined());
  assert(files.empty());

  modified = quit = running = false;

  if (!pipe.Create())
    return false;

  poll.SetMask(pipe.GetReadFD(), Poll::READ);

  return Thread::Start();
}

void
IOThread::Stop()
{
  /* set the "quit" flag and wake up the thread */
  mutex.Lock();
  quit = true;
  mutex.Unlock();
  pipe.Signal();

  /* wait for the thread to finish */
  Join();
}

void
IOThread::Add(int fd, unsigned mask, FileEventHandler &handler)
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
IOThread::Remove(int fd)
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
IOThread::LockAdd(int fd, unsigned mask, FileEventHandler &handler)
{
  mutex.Lock();
  const bool old_modified = modified;
  Add(fd, mask, handler);
  const bool new_modified = modified;
  mutex.Unlock();

  if (!old_modified && new_modified)
    pipe.Signal();
}

void
IOThread::LockRemove(int fd)
{
  mutex.Lock();
  const bool old_modified = modified;
  Remove(fd);
  const bool new_modified = modified;

  if (new_modified && running && !IsInside()) {
    /* this method is synchronous: after returning, all handlers must
       be finished */

    do {
      cond.Wait(mutex);
    } while (running);
  }

  mutex.Unlock();

  if (!old_modified && new_modified)
    pipe.Signal();
}

void
IOThread::Update()
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
      /* the iterator must be incremented before calling map::erase()
         (with its old value), because map::erase() invalidates the
         iterator */
      files.erase(i++);
    else
      ++i;
  }
}

IOThread::File *
IOThread::CollectReady()
{
  File *ready = NULL;
  for (auto i = poll.begin(), end = poll.end(); i != end; ++i) {
    const int fd = *i;
    const unsigned mask = i.GetMask();
    assert(mask != 0);

    if (fd == pipe.GetReadFD()) {
      pipe.Read();
      continue;
    }

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
IOThread::HandleReady(File *ready)
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

    mutex.Unlock();
    bool result = ready->handler->OnFileEvent(ready->fd, mask);
    mutex.Lock();

    if (!result && ready->mask != 0) {
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
IOThread::Run()
{
  assert(!running);

  mutex.Lock();

  while (!quit) {
    if (modified) {
      modified = false;
      Update();
    }

    mutex.Unlock();
    poll.Wait();
    mutex.Lock();

    if (quit)
      break;

    HandleReady(CollectReady());
  }

  mutex.Unlock();
}
