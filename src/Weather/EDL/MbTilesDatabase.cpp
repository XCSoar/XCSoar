// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MbTilesDatabase.hpp"

#include "Math/Angle.hpp"

#include <sqlite3.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace EDL {

class Statement {
  sqlite3_stmt *stmt = nullptr;

public:
  Statement(sqlite3 *db, const char *sql)
  {
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
      throw std::runtime_error(sqlite3_errmsg(db));
  }

  ~Statement() noexcept
  {
    if (stmt != nullptr)
      sqlite3_finalize(stmt);
  }

  sqlite3_stmt *get() const noexcept {
    return stmt;
  }
};

static GeoBounds
ParseBounds(const char *value) noexcept
{
  if (value == nullptr)
    return GeoBounds::Invalid();

  double west, south, east, north;
  if (sscanf(value, "%lf,%lf,%lf,%lf", &west, &south, &east, &north) != 4)
    return GeoBounds::Invalid();

  return GeoBounds({Angle::Degrees(west), Angle::Degrees(north)},
                   {Angle::Degrees(east), Angle::Degrees(south)});
}

MbTilesDatabase::MbTilesDatabase(Path path)
{
  if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
    const char *message = db != nullptr ? sqlite3_errmsg(db) : "sqlite open failed";
    if (db != nullptr) {
      sqlite3_close(db);
      db = nullptr;
    }
    throw std::runtime_error(message);
  }

  try {
    {
      Statement stmt(db, "SELECT name, value FROM metadata");
      while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        const auto *name = (const char *)sqlite3_column_text(stmt.get(), 0);
        const auto *value = (const char *)sqlite3_column_text(stmt.get(), 1);
        if (name == nullptr || value == nullptr)
          continue;

        if (strcmp(name, "format") == 0)
          metadata.format = value;
        else if (strcmp(name, "bounds") == 0)
          metadata.bounds = ParseBounds(value);
        else if (strcmp(name, "minzoom") == 0)
          metadata.min_zoom = strtoul(value, nullptr, 10);
        else if (strcmp(name, "maxzoom") == 0)
          metadata.max_zoom = strtoul(value, nullptr, 10);
      }
    }

    if (metadata.max_zoom == 0 && metadata.min_zoom == 0) {
      Statement stmt(db, "SELECT MIN(zoom_level), MAX(zoom_level) FROM tiles");
      if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        metadata.min_zoom = sqlite3_column_int(stmt.get(), 0);
        metadata.max_zoom = sqlite3_column_int(stmt.get(), 1);
      }
    }
  } catch (...) {
    sqlite3_close(db);
    db = nullptr;
    throw;
  }
}

MbTilesDatabase::MbTilesDatabase(MbTilesDatabase &&other) noexcept
  :db(other.db), metadata(other.metadata)
{
  other.db = nullptr;
}

MbTilesDatabase &
MbTilesDatabase::operator=(MbTilesDatabase &&other) noexcept
{
  if (this != &other) {
    if (db != nullptr)
      sqlite3_close(db);

    db = other.db;
    metadata = other.metadata;
    other.db = nullptr;
  }

  return *this;
}

MbTilesDatabase::~MbTilesDatabase() noexcept
{
  if (db != nullptr)
    sqlite3_close(db);
}

bool
MbTilesDatabase::HasTile(unsigned zoom, unsigned column, unsigned row) const
{
  Statement stmt(db,
                 "SELECT 1 FROM tiles "
                 "WHERE zoom_level=? AND tile_column=? AND tile_row=?");
  sqlite3_bind_int(stmt.get(), 1, zoom);
  sqlite3_bind_int(stmt.get(), 2, column);
  sqlite3_bind_int(stmt.get(), 3, row);
  return sqlite3_step(stmt.get()) == SQLITE_ROW;
}

std::vector<std::byte>
MbTilesDatabase::LoadTile(unsigned zoom, unsigned column, unsigned row) const
{
  Statement stmt(db,
                 "SELECT tile_data FROM tiles "
                 "WHERE zoom_level=? AND tile_column=? AND tile_row=?");
  sqlite3_bind_int(stmt.get(), 1, zoom);
  sqlite3_bind_int(stmt.get(), 2, column);
  sqlite3_bind_int(stmt.get(), 3, row);

  if (sqlite3_step(stmt.get()) != SQLITE_ROW)
    throw std::runtime_error("MBTiles tile not found");

  const auto *data = (const std::byte *)sqlite3_column_blob(stmt.get(), 0);
  const int size = sqlite3_column_bytes(stmt.get(), 0);
  return std::vector<std::byte>(data, data + size);
}

} // namespace EDL
