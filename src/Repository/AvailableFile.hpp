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

#ifndef XCSOAR_AVAILABLE_FILE_HPP
#define XCSOAR_AVAILABLE_FILE_HPP

#include "util/StaticString.hxx"
#include "FileType.hpp"
#include "time/BrokenDate.hpp"

#include <string>

/**
 * The description of a file that is available in a remote repository.
 */
struct AvailableFile {
  /**
   * Base name of the file.
   */
  std::string name;

  /**
   * Absolute HTTP URI.
   */
  std::string uri;

  /**
   * A short symbolic name for the area.  Empty means this file is
   * global.
   */
  NarrowString<8> area;

  FileType type;

  BrokenDate update_date;

  /**
  * The SHA256 hash of the contents of this file.
  * Zeroed if no hash is available.
  */
  std::array<std::byte, 32> sha256_hash;

  bool IsEmpty() const {
    return name.empty();
  }

  bool IsValid() const {
    return !name.empty() && !uri.empty();
  }

  bool HasHash() const {
    for (std::size_t i = 0; i < sha256_hash.size(); i++) {
      if (sha256_hash[i] != std::byte{0})
        return true;
    }

    return false;
  }

  void Clear() {
    name.clear();
    uri.clear();
    area.clear();
    type = FileType::UNKNOWN;
    update_date = BrokenDate::Invalid();
    sha256_hash.fill(std::byte{0});
  }

  const char *GetName() const {
    return name.c_str();
  }

  const char *GetURI() const {
    return uri.c_str();
  }

  const char *GetArea() const {
    return area;
  }
};

#endif
