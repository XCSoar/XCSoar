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

#ifndef XCSOAR_FILE_CACHE_HPP
#define XCSOAR_FILE_CACHE_HPP

#include "system/Path.hpp"

#include <memory>

#include <stdio.h>
#include <tchar.h>

class Reader;
class FileOutputStream;

class FileCache {
  AllocatedPath cache_path;

public:
  FileCache(AllocatedPath &&_cache_path);

protected:
  [[gnu::pure]]
  AllocatedPath MakeCachePath(const TCHAR *name) const {
    return AllocatedPath::Build(cache_path, name);
  }

public:
  void Flush(const TCHAR *name);

  /**
   * Returns nullptr on error.
   */
  std::unique_ptr<Reader> Load(const TCHAR *name, Path original_path) noexcept;

  /**
   * Throws on error.
   */
  std::unique_ptr<FileOutputStream> Save(const TCHAR *name, Path original_path);
};

#endif
