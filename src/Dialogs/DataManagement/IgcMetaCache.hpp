// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "util/StaticString.hxx"
#include "time/BrokenTime.hpp"

#include <vector>

class IgcMetaCache {
public:
  /**
   * Return compact "HH:MM - HH:MM (duration)" string for a file.
   */
  const char *GetCompactInfo(Path path) noexcept;

private:
  struct Meta {
    bool has_start{false};
    bool has_end{false};
    BrokenTime start;
    BrokenTime end;
  };

  struct CacheEntry {
    Path path;
    Meta meta;
    StaticString<64> text;
  };

  std::vector<CacheEntry> cache;

  CacheEntry *FindOrParse(Path path) noexcept;
};
