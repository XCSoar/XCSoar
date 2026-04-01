// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Sqlite.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <utility>

namespace Sqlite {

Statement::Statement(sqlite3 *db, std::string_view sql)
{
  if (sqlite3_prepare_v2(db, sql.data(), static_cast<int>(sql.size()),
                         &stmt, nullptr) != SQLITE_OK)
    throw std::runtime_error(sqlite3_errmsg(db));
}

Statement::Statement(Statement &&other) noexcept
  :stmt(std::exchange(other.stmt, nullptr)) {}

Statement::~Statement() noexcept
{
  if (stmt != nullptr)
    sqlite3_finalize(stmt);
}

bool
Statement::StepRow() const
{
  const int result = sqlite3_step(stmt);
  if (result == SQLITE_ROW)
    return true;

  if (result == SQLITE_DONE)
    return false;

  throw std::runtime_error(sqlite3_errmsg(sqlite3_db_handle(stmt)));
}

void
Statement::BindInt(int index, int value) const noexcept
{
  sqlite3_bind_int(stmt, index, value);
}

std::string_view
Statement::GetTextColumn(int index) const noexcept
{
  const auto *value = (const char *)sqlite3_column_text(stmt, index);
  return value != nullptr ? std::string_view(value) : std::string_view{};
}

int
Statement::GetIntColumn(int index) const noexcept
{
  return sqlite3_column_int(stmt, index);
}

const std::byte *
Statement::GetBlobColumn(int index) const noexcept
{
  return (const std::byte *)sqlite3_column_blob(stmt, index);
}

int
Statement::GetBytesColumn(int index) const noexcept
{
  return sqlite3_column_bytes(stmt, index);
}

Database::Database(Path path)
{
  if (sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READONLY,
                     nullptr) != SQLITE_OK) {
    const std::string message = db != nullptr
      ? sqlite3_errmsg(db)
      : "sqlite open failed";
    if (db != nullptr) {
      sqlite3_close(db);
      db = nullptr;
    }

    throw std::runtime_error(message);
  }
}

Database::Database(Database &&other) noexcept
  :db(std::exchange(other.db, nullptr)) {}

Database::~Database() noexcept
{
  if (db != nullptr)
    sqlite3_close(db);
}

Statement
Database::CreateStatement(std::string_view sql) const
{
  return Statement(db, sql);
}

} // namespace Sqlite

