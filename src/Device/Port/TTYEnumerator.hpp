/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_ENUMERATOR_HPP
#define XCSOAR_DEVICE_ENUMERATOR_HPP

#include <dirent.h>

/**
 * A class that can enumerate TTY devices on Linux and other POSIX
 * systems, by reading directory entries in /dev.
 * Reading /sys/class/tty would also haven been possible
 * but symlink created by udev rules are only visible in /dev.
 */
class TTYEnumerator {
  DIR *dir;
  char path[64];

public:
  TTYEnumerator() noexcept
    :dir(opendir("/dev")) {}

  ~TTYEnumerator() noexcept {
    if (dir != nullptr)
      closedir(dir);
  }

  /**
   * Has the constructor failed?
   */
  bool HasFailed() const noexcept {
    return dir == nullptr;
  }

  /**
   * Find the next device (or the first one, if this method hasn't
   * been called so far).
   *
   * @return the absolute path of the device, or nullptr if there are
   * no more devices
   */
  const char *Next() noexcept;
};

#endif
