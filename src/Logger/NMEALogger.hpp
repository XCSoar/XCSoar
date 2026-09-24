// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Mutex.hxx"
#include "system/Path.hpp"

#include <memory>

class FileOutputStream;

class NMEALogger {
  mutable Mutex mutex;
  std::unique_ptr<FileOutputStream> file;

  bool enabled = false;

public:
  NMEALogger() noexcept;
  ~NMEALogger() noexcept;

  bool IsEnabled() const noexcept {
    return enabled;
  }

  void Enable() noexcept {
    enabled = true;
  }

  void ToggleEnabled() noexcept {
    enabled = !enabled;
  }

  /**
   * The file being written right now, or nullptr if none is open.
   * A backup leaves it out: it is not sharable on Windows, and
   * incomplete anyway.
   */
  [[gnu::pure]]
  AllocatedPath GetPath() const noexcept;

  /**
   * Logs NMEA string to log file
   * @param text
   */
  void Log(const char *line) noexcept;

private:
  void Start();
};
