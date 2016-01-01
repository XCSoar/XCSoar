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

#include "AllInput.hpp"
#include "Event/Shared/Event.hpp"
#include "Event/Queue.hpp"
#include "IO/Async/IOLoop.hpp"
#include "Util/CharUtil.hpp"

#include <sys/inotify.h>
#include <dirent.h>

bool
AllLinuxInputDevices::Open()
{
#ifdef HAVE_INOTIFY
  if (!inotify_fd.CreateInotify())
    return false;

  /* watch /dev/input */
  int wd = inotify_add_watch(inotify_fd.Get(), "/dev/input",
                             IN_CREATE|IN_DELETE|IN_MOVED_FROM|IN_MOVED_TO|IN_ATTRIB);
  if (wd < 0) {
    inotify_fd.Close();
    return false;
  }

  io_loop.Add(inotify_fd, io_loop.READ, *this);
#endif

  DIR *dir = opendir("/dev/input");
  if (dir != nullptr) {
    dirent *e;
    while ((e = readdir(dir)) != nullptr)
      Add(e->d_name);

    closedir(dir);
  }

  return true;
}

void
AllLinuxInputDevices::Close()
{
  devices.clear();

#ifdef HAVE_INOTIFY
  if (inotify_fd.IsDefined()) {
    io_loop.Remove(inotify_fd);
    inotify_fd.Close();
  }
#endif
}

std::list<AllLinuxInputDevices::Device>::iterator
AllLinuxInputDevices::FindByName(const char *name)
{
  const auto end = devices.end();
  for (auto i = devices.begin(); i != end; ++i)
    if (i->name == name)
      return i;

  return end;
}

bool
AllLinuxInputDevices::CheckName(const char *name)
{
  return memcmp(name, "event", 5) == 0 && IsDigitASCII(name[5]);
}

void
AllLinuxInputDevices::Add(const char *name)
{
  if (!CheckName(name))
    return;

#ifdef HAVE_INOTIFY
  auto i = FindByName(name);
  if (i != devices.end())
    /* already have that one */
    return;
#endif

  StaticString<64> path;
  path.Format("/dev/input/%s", name);

  devices.emplace_back(name, io_loop, queue, merge);
  if (!devices.back().device.Open(path))
    devices.pop_back();
}

#ifdef HAVE_INOTIFY

void
AllLinuxInputDevices::Remove(const char *name)
{
  if (!CheckName(name))
    return;

  auto i = FindByName(name);
  if (i != devices.end())
    devices.erase(i);
}

inline void
AllLinuxInputDevices::Read()
{
  uint8_t buffer[1024];
  const ssize_t nbytes = inotify_fd.Read(buffer, sizeof(buffer));
  if (nbytes < 0)
    return;

  for (size_t i = 0; i < size_t(nbytes);) {
    const void *p = buffer + i;
    const inotify_event &e = *(const inotify_event *)p;
    i += sizeof(e) + e.len;

    if (e.len > 0) {
      if ((e.mask & (IN_DELETE|IN_MOVED_FROM)) != 0)
        Remove(e.name);

      /* when udev creates a new device node, it is owned by root, and
         XCSoar may attempt to open it before the permissions had been
         adjusted, therefore we try again on IN_ATTRIB to avoid this
         race condition */

      if ((e.mask & (IN_CREATE|IN_MOVED_TO|IN_ATTRIB)) != 0)
        Add(e.name);
    }
  }
}

bool
AllLinuxInputDevices::OnFileEvent(FileDescriptor fd, unsigned mask)
{
  Read();

  return true;
}

#endif
