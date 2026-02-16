// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <memory>
#include <stdio.h>
#include <tchar.h>

class Reader;
class FileOutputStream;

class FileCache {
  AllocatedPath cache_path;

public:
  FileCache(AllocatedPath &&_cache_path);

protected:
  [[gnu::pure]]
  AllocatedPath MakeCachePath(const char *name) const {
    return AllocatedPath::Build(cache_path, name);
  }

public:
  void Flush(const char *name);

  /**
   * Returns nullptr on error.
   */
  std::unique_ptr<Reader> Load(const char *name, Path original_path) noexcept;

  /**
   * Throws on error.
   */
  std::unique_ptr<FileOutputStream> Save(const char *name, Path original_path);
};
