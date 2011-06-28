/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_MAP_WINDOW_LABELS_HPP
#define XCSOAR_MAP_WINDOW_LABELS_HPP

#include "Screen/TextInBox.hpp"
#include "Screen/Point.hpp"
#include "Util/NonCopyable.hpp"
#include "Sizes.h" /* for NAME_SIZE */

#include <tchar.h>

class WaypointLabelList : private NonCopyable {
public:
  struct Label{
    TCHAR Name[NAME_SIZE+1];
    RasterPoint Pos;
    TextInBoxMode_t Mode;
    int AltArivalAGL;
    bool inTask;
    bool isLandable;
    bool isAirport;
    bool isWatchedWaypoint;
  };

protected:
  unsigned width, height;
  PixelRect bounds;
  Label labels[128];
  unsigned num_labels;

public:
  WaypointLabelList(unsigned _width, unsigned _height)
    :width(_width), height(_height), num_labels(0) {}

  void Add(const TCHAR *Name, int X, int Y, TextInBoxMode_t Mode,
           int AltArivalAGL, bool inTask, bool isLandable, bool isAirport,
           bool isWatchedWaypoint);
  void Sort();

  unsigned size() const {
    return num_labels;
  }

  const Label &operator[](unsigned i) const {
    return labels[i];
  }
};

#endif
