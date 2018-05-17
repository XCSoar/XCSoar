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

#ifndef XCSOAR_FLARM_COLOR_DATABASE_HPP
#define XCSOAR_FLARM_COLOR_DATABASE_HPP

#include "FlarmId.hpp"
#include "Color.hpp"
#include "Compiler.h"

#include <map>

#include <assert.h>

class FlarmColorDatabase {
  typedef std::map<FlarmId, FlarmColor> Map;
  Map data;

public:
  typedef Map::const_iterator const_iterator;

  gcc_pure
  const_iterator begin() const {
    return data.begin();
  }

  gcc_pure
  const_iterator end() const {
    return data.end();
  }

  gcc_pure
  FlarmColor Get(FlarmId id) const {
    auto i = data.find(id);
    if (i == data.end())
      return FlarmColor::NONE;

    return i->second;
  }

  void Set(FlarmId id, FlarmColor color) {
    assert(color != FlarmColor::NONE);
    assert(color != FlarmColor::COUNT);

    auto i = data.insert(std::make_pair(id, color));
    if (!i.second)
      /* the id already exists in the map: overwrite the old color */
      i.first->second = color;
  }

  void Remove(FlarmId id) {
    data.erase(id);
  }
};

#endif
