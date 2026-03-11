// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoBounds.hpp"
#include "system/Path.hpp"
#include "util/StaticString.hxx"

#include <cstddef>
#include <span>
#include <vector>

struct sqlite3;

namespace EDL {

struct MbTilesMetadata {
  StaticString<32> format;
  GeoBounds bounds = GeoBounds::Invalid();
  unsigned min_zoom = 0;
  unsigned max_zoom = 0;
};

class MbTilesDatabase {
  sqlite3 *db = nullptr;
  MbTilesMetadata metadata;

public:
  explicit MbTilesDatabase(Path path);
  MbTilesDatabase(MbTilesDatabase &&other) noexcept;
  MbTilesDatabase &operator=(MbTilesDatabase &&other) noexcept;
  ~MbTilesDatabase() noexcept;

  MbTilesDatabase(const MbTilesDatabase &) = delete;
  MbTilesDatabase &operator=(const MbTilesDatabase &) = delete;

  const MbTilesMetadata &GetMetadata() const noexcept {
    return metadata;
  }

  [[gnu::pure]]
  bool
  HasTile(unsigned zoom, unsigned column, unsigned row) const;

  std::vector<std::byte>
  LoadTile(unsigned zoom, unsigned column, unsigned row) const;
};

} // namespace EDL
