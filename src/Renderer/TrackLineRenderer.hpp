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

#ifndef XCSOAR_TRACK_LINE_RENDERER_HPP
#define XCSOAR_TRACK_LINE_RENDERER_HPP

struct PixelPoint;
class Canvas;
class Angle;
class WindowProjection;
struct MapLook;
struct NMEAInfo;
struct DerivedInfo;
struct MapSettings;

/**
 * Renderer for the track line
 * Renders line forward along the current ground track (not heading) in straight flight
 * or curve using current rate of curve when curving / circling.
 */
class TrackLineRenderer {
  const MapLook &look;

public:
  TrackLineRenderer(const MapLook &_look):look(_look) {}

  void Draw(Canvas &canvas, const Angle screen_angle, const Angle track_angle,
            PixelPoint pos);

  void Draw(Canvas &canvas,
            const WindowProjection &projection,
            PixelPoint pos, const NMEAInfo &basic,
            const DerivedInfo &calculated, const MapSettings &settings,
            bool wind_relative);

protected:
  void DrawProjected(Canvas &canvas,
                     const WindowProjection &projection,
                     const NMEAInfo &basic,
                     const DerivedInfo &calculated,
                     const MapSettings &settings,
                     bool wind_relative);
};

#endif
