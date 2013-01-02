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

#ifndef XCSOAR_WAYPOINT_LABEL_LIST_HPP
#define XCSOAR_WAYPOINT_LABEL_LIST_HPP

#include "Rough/RoughAltitude.hpp"
#include "Renderer/TextInBox.hpp"
#include "Screen/Point.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/StaticArray.hpp"
#include "Sizes.h" /* for NAME_SIZE */

#include <tchar.h>

class WaypointLabelList : private NonCopyable {
public:
  struct Label{
    TCHAR Name[NAME_SIZE+1];
    RasterPoint Pos;
    TextInBoxMode Mode;
    RoughAltitude AltArivalAGL;
    bool inTask;
    bool isLandable;
    bool isAirport;
    bool isWatchedWaypoint;
    bool bold;
  };

protected:
  UPixelScalar width, height;
  PixelRect bounds;

  StaticArray<Label, 128u> labels;

public:
  WaypointLabelList(UPixelScalar _width, UPixelScalar _height)
    :width(_width), height(_height) {}

  void Add(const TCHAR *Name, PixelScalar X, PixelScalar Y,
           TextInBoxMode Mode, bool bold,
           RoughAltitude AltArivalAGL,
           bool inTask, bool isLandable, bool isAirport,
           bool isWatchedWaypoint);
  void Sort();

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
