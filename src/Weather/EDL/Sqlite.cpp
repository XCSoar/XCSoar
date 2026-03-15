// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Sqlite.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <utility>

namespace EDL {

SqliteStatement::SqliteStatement(sqlite3 *db, std::string_view sql)
{
  if (sqlite3_prepare_v2(db, sql.data(), static_cast<int>(sql.size()),
                         &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
}

SqliteStatement::SqliteStatement(SqliteStatement &&other) noexcept
  :stmt(std::exchange(other.stmt, nullptr)) {}

SqliteStatement::~SqliteStatement() noexcept
{
  if (stmt != nullptr)
    sqlite3_finalize(stmt);
}

bool
SqliteStatement::StepRow() const noexcept
{
  return sqlite3_step(stmt) == SQLITE_ROW;
}

void
SqliteStatement::BindInt(int index, int value) const noexcept
{
  sqlite3_bind_int(stmt, index, value);
}

std::string_view
SqliteStatement::GetTextColumn(int index) const noexcept
{
  const auto *value = (const char *)sqlite3_column_text(stmt, index);
  return value != nullptr ? std::string_view(value) : std::string_view{};
}

int
SqliteStatement::GetIntColumn(int index) const noexcept
{
  return sqlite3_column_int(stmt, index);
}

const std::byte *
SqliteStatement::GetBlobColumn(int index) const noexcept
{
  return (const std::byte *)sqlite3_column_blob(stmt, index);
}

int
SqliteStatement::GetBytesColumn(int index) const noexcept
{
  return sqlite3_column_bytes(stmt, index);
}

SqliteDatabase::SqliteDatabase(Path path)
{
  if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
    const char *message = db != nullptr ? sqlite3_errmsg(db) : "sqlite open failed";
    if (db != nullptr) {
      sqlite3_close(db);
      db = nullptr;
    }

    throw std::runtime_error(message);
  }
}

SqliteDatabase::SqliteDatabase(SqliteDatabase &&other) noexcept
  :db(std::exchange(other.db, nullptr)) {}

SqliteDatabase::~SqliteDatabase() noexcept
{
  if (db != nullptr)
    sqlite3_close(db);
}

SqliteStatement
SqliteDatabase::CreateStatement(std::string_view sql) const
{
  return SqliteStatement(db, sql);
}

} // namespace EDL
