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

#ifndef XCSOAR_IO_ZIP_ARCHIVE_HPP
#define XCSOAR_IO_ZIP_ARCHIVE_HPP

#include "Compiler.h"

#include <algorithm>
#include <string>
#include <cstddef>

class Path;

/**
 * A handle to z ZIP archive file.  It is a OO wrapper for struct
 * zzip_dir.
 */
class ZipArchive {
  struct zzip_dir *dir = nullptr;

public:
  /**
   * Open a ZIP archive.  Throws std::runtime_error on error.
   */
  explicit ZipArchive(Path path);
  ~ZipArchive();

  ZipArchive(ZipArchive &&src):dir(src.dir) {
    src.dir = nullptr;
  }

  ZipArchive &operator=(ZipArchive &&src) {
    std::swap(dir, src.dir);
    return *this;
  }

  struct zzip_dir *get() {
    return dir;
  }

  gcc_pure
  bool Exists(const char *name) const;

  /**
   * Obtain the next directory entry name.  Can be used to iterate
   * over all files in the archive.  Returns an empty string after the
   * last entry.
   */
  std::string NextName();
};

#endif
