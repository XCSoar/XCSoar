// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlarmId.hpp"
#include "Color.hpp"

#include <map>
#include <cassert>

class FlarmColorDatabase {
  typedef std::map<FlarmId, FlarmColor> Map;
  Map data;

public:
  typedef Map::const_iterator const_iterator;

  [[gnu::pure]]
  const_iterator begin() const {
    return data.begin();
  }

  [[gnu::pure]]
  const_iterator end() const {
    return data.end();
  }

  [[gnu::pure]]
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
