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

#include "MapWindow.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/TextInBox.hpp"
#include "Renderer/WindArrowRenderer.hpp"
#include "Look/Look.hpp"

/**
 * Draws the FANET wind_sock icons onto the given canvas
 * Based on MapWindowTraffic.cpp
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawFANETWeather(Canvas &canvas,
                            const PixelPoint aircraft_pos) const
{
  StationsList stations_list = Basic().fanet.stations;
  if(stations_list.IsEmpty())
    return;

  const WindowProjection &projection = render_projection;

  // If zoomed in too far out, dont draw station
  if (projection.GetMapScale() > 7500) {
    return;
  }

  // Draw all stations
  for (const auto & station : stations_list.list)
  {
    if ( ! station.location_available)
      continue;

    GeoPoint station_loc = station.location;
    PixelPoint sc, sc_wind_info;

    // If FANET station not on the screen, continue to the next one
    if ( ! projection.GeoToScreenIfVisible(station_loc, sc))
      continue;

    // Wind angle relative to the screen orientation
    Angle ang = Angle::Degrees(station.wind_dir_deg);
    auto angle = ang - projection.GetScreenAngle();

    // Draw wind info box(speed|gust)
    sc_wind_info = sc;

    // If arrow pointing down move info box closer vertically
    if (angle.Between(Angle::Degrees(280),
                           Angle::Degrees(80)))
      sc_wind_info.y += Layout::Scale(8);
    // If arrow is pointing up move it further away vertically
    else if (angle.Between(Angle::Degrees(110),
                      Angle::Degrees(250)))
      sc_wind_info.y += Layout::Scale(18);
    /*
     * We are left with the 20deg above and below the horizon on both sides,
     * and because of the angle of the arrow we need to move it slightly lower
     * vertically so that arrow doesnt overlap wind info box
     */
    else
      sc_wind_info.y += Layout::Scale(13);

    TextInBoxMode mode_wind_info;
    mode_wind_info.shape = LabelShape::ROUNDED_BLACK;
    mode_wind_info.align = TextInBoxMode::CENTER;
    mode_wind_info.vertical_position = TextInBoxMode::VerticalPosition::CENTERED;

    // If station is close to aircraft dont show wind info box
    int dx = sc_wind_info.x - aircraft_pos.x;
    int dy = sc_wind_info.y - aircraft_pos.y;
    if (dx * dx + dy * dy > Layout::Scale(30 * 30))
    {
      StaticString<8> text_wind_info;
      text_wind_info.Format(_T("%.0f|%.0f"),
                    station.wind_speed_kmph, station.wind_gust_kmph);

      TextInBox(canvas, text_wind_info, sc_wind_info.x,
                sc_wind_info.y, mode_wind_info, GetClientRect(), nullptr);
    }

    auto style = GetMapSettings().wind_arrow_style;
    WindArrowRenderer renderer(look.wind);
    renderer.DrawArrow(canvas, sc, angle, 10, style, 0);
  }
}
