/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_OS_FILE_MAPPING_HPP
#define XCSOAR_OS_FILE_MAPPING_HPP

#include <cstddef>

#ifndef HAVE_POSIX
#include <windef.h>
#endif

class Path;

/**
 * Maps a file into the address space of this process.
 */
class FileMapping {
  void *m_data = nullptr;
  size_t m_size;

#ifndef HAVE_POSIX
  HANDLE hFile, hMapping = nullptr;
#endif

public:
  /**
   * Throws on error.
   */
  FileMapping(Path path);

  ~FileMapping() noexcept;

  FileMapping(const FileMapping &) = delete;
  FileMapping &operator=(const FileMapping &) = delete;

  /**
   * Returns a pointer to the beginning of the mapping.
   */
  const void *data() const noexcept {
    return m_data;
  }

  const void *at(size_t offset) const noexcept {
    return (const char *)data() + offset;
  }

  const void *end() const noexcept {
    return at(size());
  }

  /**
   * Returns the size of the file, in bytes.
   */
  size_t size() const noexcept {
    return m_size;
  }
};

#endif
