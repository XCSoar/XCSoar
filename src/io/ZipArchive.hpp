// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <algorithm>
#include <string>
#include <cstddef>

class Path;

/**
 * A handle to z ZIP archive file.  It is a OO wrapper for struct
 * zzip_dir.
 */
class ZipArchive {
  struct zzip_dir *dir = nullptr;

public:
  /**
   * Open a ZIP archive.  Throws std::runtime_error on error.
   */
  explicit ZipArchive(Path path);
  ~ZipArchive() noexcept;

  ZipArchive(ZipArchive &&src) noexcept:dir(src.dir) {
    src.dir = nullptr;
  }

  ZipArchive &operator=(ZipArchive &&src) noexcept {
    std::swap(dir, src.dir);
    return *this;
  }

  struct zzip_dir *get() noexcept {
    return dir;
  }

  [[gnu::pure]]
  bool Exists(const char *name) const noexcept;

  /**
   * Obtain the next directory entry name.  Can be used to iterate
   * over all files in the archive.  Returns an empty string after the
   * last entry.
   */
  std::string NextName() noexcept;
};
