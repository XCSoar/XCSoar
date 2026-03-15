// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <cstddef>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace EDL {

class SqliteStatement {
  sqlite3_stmt *stmt = nullptr;

public:
  SqliteStatement(sqlite3 *db, std::string_view sql);
  SqliteStatement(SqliteStatement &&other) noexcept;
  ~SqliteStatement() noexcept;

  SqliteStatement(const SqliteStatement &) = delete;
  SqliteStatement &operator=(const SqliteStatement &) = delete;

  bool StepRow() const noexcept;
  void BindInt(int index, int value) const noexcept;
  std::string_view GetTextColumn(int index) const noexcept;
  int GetIntColumn(int index) const noexcept;
  const std::byte *GetBlobColumn(int index) const noexcept;
  int GetBytesColumn(int index) const noexcept;
};

class SqliteDatabase {
  sqlite3 *db = nullptr;

public:
  SqliteDatabase() = default;
  explicit SqliteDatabase(Path path);
  SqliteDatabase(SqliteDatabase &&other) noexcept;
  ~SqliteDatabase() noexcept;

  SqliteDatabase(const SqliteDatabase &) = delete;
  SqliteDatabase &operator=(const SqliteDatabase &) = delete;

  SqliteStatement CreateStatement(std::string_view sql) const;
};

} // namespace EDL
