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

#ifndef XCSOAR_AIRSPACE_LIST_RENDERER_HPP
#define XCSOAR_AIRSPACE_LIST_RENDERER_HPP

class Canvas;
class AbstractAirspace;
class TwoTextRowsRenderer;
struct PixelRect;
struct GeoVector;
struct AirspaceLook;
struct AirspaceRendererSettings;

namespace AirspaceListRenderer
{
  /**
   * Draws an airspace list item.
   *
   * Comment is e.g. "Class C"
   */
  void Draw(Canvas &canvas, const PixelRect rc, const AbstractAirspace &airspace,
            const TwoTextRowsRenderer &row_renderer,
            const AirspaceLook &look,
            const AirspaceRendererSettings &renderer_settings);

  /**
   * Draws an airspace list item.
   *
   * Comment is e.g. "Class C - 20.0 km - 56 deg"
   *
   * @param vector The distance and direction that should be
   * added to the comment
   */
  void Draw(Canvas &canvas, const PixelRect rc, const AbstractAirspace &airspace,
            const GeoVector &vector,
            const TwoTextRowsRenderer &row_renderer,
            const AirspaceLook &look,
            const AirspaceRendererSettings &renderer_settings);
}

#endif
