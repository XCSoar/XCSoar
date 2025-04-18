// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <span>

#ifndef HAVE_POSIX
#include <windef.h>
#endif

class Path;

/**
 * Maps a file into the address space of this process.
 */
class FileMapping {
  std::span<std::byte> span;

#ifndef HAVE_POSIX
  HANDLE hFile, hMapping;
#endif

public:
  /**
   * Throws on error.
   */
  FileMapping(Path path);

  ~FileMapping() noexcept;

  FileMapping(const FileMapping &) = delete;
  FileMapping &operator=(const FileMapping &) = delete;

  operator std::span<const std::byte>() const noexcept {
    return span;
  }
};
