// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <cstddef>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace Sqlite {

class Statement {
  sqlite3_stmt *stmt = nullptr;

public:
  Statement(sqlite3 *db, std::string_view sql);
  Statement(Statement &&other) noexcept;
  ~Statement() noexcept;

  Statement(const Statement &) = delete;
  Statement &operator=(const Statement &) = delete;

  bool StepRow() const;
  void BindInt(int index, int value) const noexcept;
  std::string_view GetTextColumn(int index) const noexcept;
  int GetIntColumn(int index) const noexcept;
  const std::byte *GetBlobColumn(int index) const noexcept;
  int GetBytesColumn(int index) const noexcept;
};

class Database {
  sqlite3 *db = nullptr;

public:
  Database() = default;
  explicit Database(Path path);
  Database(Database &&other) noexcept;
  ~Database() noexcept;

  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;

  Statement CreateStatement(std::string_view sql) const;
};

} // namespace Sqlite

