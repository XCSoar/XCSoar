// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileTransaction.hpp"
#include "lib/fmt/PathFormatter.hpp"
#include "lib/fmt/SystemError.hxx"
#include "system/FileUtil.hpp"

#include <cassert>

static AllocatedPath
MakeTemporaryPath(Path path) noexcept
{
  assert(path != nullptr);

#ifdef HAVE_POSIX
  return path + ".tmp";
#else
  return path.WithSuffix(".tmp");
#endif
}

FileTransaction::FileTransaction(Path _path) noexcept
  :final_path(_path), temporary_path(MakeTemporaryPath(_path))
{
  /* ensure the temporary file doesn't exist already */
  File::Delete(temporary_path);
}

FileTransaction::~FileTransaction() noexcept
{
  if (temporary_path != nullptr)
    /* cancel the transaction */
    File::Delete(temporary_path);
}

void
FileTransaction::Commit()
{
  assert(temporary_path != nullptr);

  if (!File::Replace(temporary_path, final_path)) {
#ifdef HAVE_POSIX
    throw FmtErrno("Failed to commit %s", temporary_path);
#else
    throw FmtLastError("Failed to commit {}", temporary_path);
#endif
  }

  /* mark the transaction as "finished" to avoid deletion in the
     destructor */
  temporary_path = nullptr;
}

void
FileTransaction::Abandon() noexcept
{
  assert(temporary_path != nullptr);

  temporary_path = nullptr;
}
