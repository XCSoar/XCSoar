// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Switches the Linux TTY to graphics mode.
 */
class LinuxGraphicsTTY {
  static constexpr const char *path = "/dev/tty";

  const int fd;

public:
  LinuxGraphicsTTY() noexcept;
  ~LinuxGraphicsTTY() noexcept;

  LinuxGraphicsTTY(const LinuxGraphicsTTY &) = delete;
  LinuxGraphicsTTY &operator=(const LinuxGraphicsTTY &) = delete;
};
