/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_IO_BINARY_WRITER_HPP
#define XCSOAR_IO_BINARY_WRITER_HPP

#include "FileHandle.hpp"

#ifdef _UNICODE
#include <tchar.h>
#endif

class BinaryWriter {
protected:
  FileHandle file;

public:
  /**
   * Creates a new file. 
   * Truncates the old file if it exists, unless the parameter "append" is true.
   */
  BinaryWriter(const char *path, bool append = false);

#ifdef _UNICODE
  BinaryWriter(const TCHAR *path, bool append = false);
#endif

  /**
   * Returns true if opening the file has failed.  This must be
   * checked before calling any other method.
   */
  bool HasError() const {
    return !file.IsOpen();
  }

  /**
   * Ensure that all pending writes have been passed to the operating
   * system.  This does not guarantee that these have been written to
   * the physical device; they might still reside in the filesystem
   * cache.
   */
  bool Flush() {
    return file.Flush();
  }

  bool Write(const void *s, size_t size, size_t length) {
    return file.Write(s, size, length) == length;
  }
};

#endif
