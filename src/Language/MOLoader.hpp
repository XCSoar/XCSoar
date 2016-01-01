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

#ifndef XCSOAR_MO_LOADER_HPP
#define XCSOAR_MO_LOADER_HPP

#include "MOFile.hpp"
#include "OS/FileMapping.hpp"
#include "OS/Path.hpp"

/**
 * Loader for GNU gettext *.mo files.
 */
class MOLoader {
  FileMapping *mapping;
  MOFile *file;

public:
  MOLoader(const void *data, size_t size)
    :mapping(NULL), file(new MOFile(data, size)) {}

  explicit MOLoader(Path path)
    :mapping(new FileMapping(path)),
     file(mapping->error()
          ? NULL : new MOFile(mapping->data(), mapping->size())) {
  }

  ~MOLoader() {
    delete file;
    delete mapping;
  }

  bool error() const {
    return file == NULL || file->error();
  }

  const MOFile &get() const {
    return *file;
  }
};

#endif
