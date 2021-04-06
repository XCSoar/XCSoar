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

#ifndef XCSOAR_WAYPOINT_LABEL_LIST_HPP
#define XCSOAR_WAYPOINT_LABEL_LIST_HPP

#include "Renderer/TextInBox.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/Rect.hpp"
#include "util/NonCopyable.hpp"
#include "util/StaticArray.hxx"
#include "Sizes.h" /* for NAME_SIZE */

#include <tchar.h>

class WaypointLabelList : private NonCopyable {
  static constexpr int WPCIRCLESIZE = 2;

public:
  struct Label{
    TCHAR Name[NAME_SIZE+1];
    PixelPoint Pos;
    TextInBoxMode Mode;
    int AltArivalAGL;
    bool inTask;
    bool isLandable;
    bool isAirport;
    bool isWatchedWaypoint;
    bool bold;
  };

protected:
  PixelRect clip_rect;

  StaticArray<Label, 256u> labels;

public:
  explicit WaypointLabelList(PixelRect _rect) noexcept
    :clip_rect(_rect)
  {
    clip_rect.Grow(WPCIRCLESIZE);
    clip_rect.right += WPCIRCLESIZE * 2;
  }

  void Add(const TCHAR *name, PixelPoint p,
           TextInBoxMode Mode, bool bold,
           int AltArivalAGL,
           bool inTask, bool isLandable, bool isAirport,
           bool isWatchedWaypoint) noexcept;
  void Sort() noexcept;

  const Label *begin() const noexcept {
    return labels.begin();
  }

  const Label *end() const noexcept {
    return labels.end();
  }
};

#endif
