// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Mutex.hxx"

#include <memory>

class FileOutputStream;

class NMEALogger {
  Mutex mutex;
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
   * Logs NMEA string to log file
   * @param text
   */
  void Log(const char *line) noexcept;

private:
  void Start();
};
