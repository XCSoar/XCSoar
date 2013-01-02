/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Util/StaticString.hpp"

#include <string>

#include <stdint.h>

/**
 * The description of a file that is available in a remote repository.
 */
struct AvailableFile {
  enum class Type : uint8_t {
    UNKNOWN,
    AIRSPACE,
    WAYPOINT,
    MAP,
    FLARMNET,
  };

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

  Type type;

  bool IsEmpty() const {
    return name.empty();
  }

  bool IsValid() const {
    return !name.empty() && !uri.empty();
  }

  void Clear() {
    name.clear();
    uri.clear();
    area.clear();
    type = Type::UNKNOWN;
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
