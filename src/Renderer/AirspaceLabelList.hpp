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

#ifndef XCSOAR_AIRSPACE_LABEL_LIST_HPP
#define XCSOAR_AIRSPACE_LABEL_LIST_HPP

#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Engine/Airspace/AirspaceClass.hpp"
#include "Geo/GeoPoint.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/StaticArray.hxx"

struct AirspaceWarningConfig;

class AirspaceLabelList : private NonCopyable {
public:
  struct Label {
    GeoPoint pos;
    AirspaceClass cls;
    AirspaceAltitude base;
    AirspaceAltitude top;
  };

protected:
  StaticArray<Label, 512u> labels;

public:
  AirspaceLabelList() {}

  void Add(const GeoPoint &pos, AirspaceClass cls, const AirspaceAltitude &base,
           const AirspaceAltitude &top);
  void Sort(const AirspaceWarningConfig &config);

  void Clear() {
    labels.clear();
  }

  const Label *begin() const {
    return labels.begin();
  }

  const Label *end() const {
    return labels.end();
  }

  const Label &operator[](unsigned i) const {
    return labels[i];
  }
};

#endif
