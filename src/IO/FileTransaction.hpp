/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_FILE_TRANSACTION_HPP
#define XCSOAR_FILE_TRANSACTION_HPP

#include "OS/Path.hpp"

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
  FileTransaction(Path _path);

  /**
   * The destructor auto-rolls back the transaction (i.e. deletes the
   * temporary file) unless Commit() has been called.
   */
  ~FileTransaction();

  template<typename P>
  void SetPath(P &&_path) {
    final_path = std::forward<P>(_path);
  }

  /**
   * Returns the temporary path.  This is the path that shall be used
   * by the caller to write the file.
   */
  Path GetTemporaryPath() const {
    return temporary_path;
  }

  /**
   * Replace the file with the contents of the temporary file.
   *
   * @return true on success
   */
  bool Commit();

  /**
   * Abandon the transaction, i.e. close it, but don't clean up the
   * temporary file.
   */
  void Abandon();
};

#endif
