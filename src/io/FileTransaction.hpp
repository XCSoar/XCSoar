// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <utility>

/**
 * Write to a temporary file, and then replace the old file
 * atomically.  If something fails in between, the old file remains in
 * place (or none, if it doesn't exist previously).
 *
 * To use this class, create an instance and write to the file
 * specified by GetTemporaryPath().  After the file is finished, call
 * Commit().  The destructor will automatically delete the temporary
 * file if you decide not to call Commit().
 */
class FileTransaction {
  AllocatedPath final_path;
  AllocatedPath temporary_path;

public:
  FileTransaction(Path _path) noexcept;

  /**
   * The destructor auto-rolls back the transaction (i.e. deletes the
   * temporary file) unless Commit() has been called.
   */
  ~FileTransaction() noexcept;

  template<typename P>
  void SetPath(P &&_path) noexcept {
    final_path = std::forward<P>(_path);
  }

  /**
   * Returns the temporary path.  This is the path that shall be used
   * by the caller to write the file.
   */
  Path GetTemporaryPath() const noexcept {
    return temporary_path;
  }

  /**
   * Replace the file with the contents of the temporary file.
   *
   * Throws on error.
   */
  void Commit();

  /**
   * Abandon the transaction, i.e. close it, but don't clean up the
   * temporary file.
   */
  void Abandon() noexcept;
};
