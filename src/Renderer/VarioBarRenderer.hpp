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

#ifndef XCSOAR_VARIO_BAR_RENDERER_HPP
#define XCSOAR_VARIO_BAR_RENDERER_HPP

struct MoreData;
struct PixelRect;
class Canvas;
struct DerivedInfo;
struct VarioBarLook;
class GlidePolar;

class VarioBarRenderer {
  const VarioBarLook &look;

public:
  VarioBarRenderer(const VarioBarLook &_look)
    :look(_look) {}

  const VarioBarLook &GetLook() const {
    return look;
  }

  void Draw(Canvas &canvas, const PixelRect &rc,
            const MoreData &basic,
            const DerivedInfo &calculated,
            const GlidePolar &glide_polar,
            const bool vario_bar_avg_enabled) const;
};

#endif
