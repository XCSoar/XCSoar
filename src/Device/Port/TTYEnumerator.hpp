// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <dirent.h>

/**
 * A class that can enumerate TTY devices on Linux and other POSIX
 * systems, by reading directory entries in /dev/.
 */
class TTYEnumerator {
  DIR *dir;
  char path[64];

public:
#ifdef __linux__
  /* on Linux, enumerate /sys/class/tty/ which is faster than /dev/
     (searches only the TTY character devices) and shows only the
     ports that really exist */
  /* but use /dev because there is no noticable downside, and it
   * allows for custom port mapping using udev file
   */
  TTYEnumerator() noexcept
    :dir(opendir("/dev")) {}
#else
  TTYEnumerator() noexcept
    :dir(opendir("/dev")) {}
#endif

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
