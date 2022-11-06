/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/NonCopyable.hpp"
#include "util/StaticArray.hxx"

#include <type_traits>

struct AirspaceWarningConfig;

class AirspaceLabelList : private NonCopyable {
public:
  struct Label {
    GeoPoint pos;
    AirspaceClass cls;
    AirspaceAltitude base;
    AirspaceAltitude top;
  };

  static_assert(std::is_trivial_v<Label>);

protected:
  StaticArray<Label, 512u> labels;

public:
  void Add(const GeoPoint &pos, AirspaceClass cls, const AirspaceAltitude &base,
           const AirspaceAltitude &top) noexcept;
  void Sort(const AirspaceWarningConfig &config) noexcept;

  void Clear() noexcept {
    labels.clear();
  }

  auto begin() const noexcept {
    return labels.begin();
  }

  auto end() const noexcept {
    return labels.end();
  }

  const Label &operator[](unsigned i) const noexcept {
    return labels[i];
  }
};
