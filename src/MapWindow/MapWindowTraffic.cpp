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

#include "MapWindow.hpp"
#include "Math/Screen.hpp"
#include "Math/Earth.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Screen/TextInBox.hpp"
#include "StringUtil.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Look/TrafficLook.hpp"

#include <stdio.h>

/**
 * Draws the FLARM traffic icons onto the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawFLARMTraffic(Canvas &canvas,
                            const RasterPoint aircraft_pos) const
{
  // Return if FLARM icons on moving map are disabled
  if (!SettingsMap().EnableFLARMMap)
    return;

  // Return if FLARM data is not available
  const FLARM_STATE &flarm = Basic().flarm;
  if (!flarm.available)
    return;

  const MapWindowProjection &projection = render_projection;

  // if zoomed in too far out, dont draw traffic since it will be too close to
  // the glider and so will be meaningless (serves only to clutter, cant help
  // the pilot)
  if ((projection.GetMapScale()> fixed_int_constant(7300)) && (flarm.GetActiveTrafficCount()>0)) {
    return;
  }

  // Create point array that will form that arrow polygon
  RasterPoint Arrow[5];

  // Circle through the FLARM targets
  for (unsigned i = 0; i < flarm.traffic.size(); i++) {
    const FLARM_TRAFFIC &traffic = flarm.traffic[i];

    if (!traffic.location_available)
      continue;

    // Save the location of the FLARM target
    GeoPoint target_loc = traffic.location;

    // Points for the screen coordinates for the icon, name and average climb
    RasterPoint sc, sc_name, sc_av;

    // If FLARM target not on the screen, move to the next one
    if (!projection.GeoToScreenIfVisible(target_loc, sc))
      continue;

    // Draw the name 16 points below the icon
    sc_name = sc;
    sc_name.y -= Layout::Scale(20);

    // Draw the average climb value above the icon
    sc_av = sc;
    sc_av.y += Layout::Scale(5);

    TextInBoxMode mode;
    mode.Mode = Outlined;

    // JMW TODO enhancement: decluttering of FLARM altitudes (sort by max lift)

    int dx = sc_av.x - aircraft_pos.x;
    int dy = sc_av.y - aircraft_pos.y;

    // only draw labels if not close to aircraft
    if (dx * dx + dy * dy > Layout::Scale(30 * 30)) {
      // If FLARM callsign/name available draw it to the canvas
      if (traffic.HasName() && !string_is_empty(traffic.name))
        TextInBox(canvas, traffic.name, sc_name.x, sc_name.y,
                  mode, get_client_rect());

      if (traffic.climb_rate_avg30s >= fixed(0.1)) {
        // If average climb data available draw it to the canvas
        TCHAR label_avg[100];
        Units::FormatUserVSpeed(traffic.climb_rate_avg30s, label_avg, 100, false);
        TextInBox(canvas, label_avg, sc_av.x, sc_av.y, mode, get_client_rect());
      }
    }

    // If FLARM alarm draw alarm icon below corresponding target
    if ((traffic.alarm_level > 0) && (traffic.alarm_level < 4))
      traffic_look.alert_icon.draw(canvas, sc);

    // Fill the Arrow array with a normal arrow pointing north
    Arrow[0].x = -4;
    Arrow[0].y = 6;
    Arrow[1].x = 0;
    Arrow[1].y = -8;
    Arrow[2].x = 4;
    Arrow[2].y = 6;
    Arrow[3].x = 0;
    Arrow[3].y = 3;
    Arrow[4].x = -4;
    Arrow[4].y = 6;

    // Select brush depending on AlarmLevel
    switch (traffic.alarm_level) {
    case FLARM_TRAFFIC::ALARM_LOW:
      canvas.select(traffic_look.warning_brush);
      break;
    case FLARM_TRAFFIC::ALARM_IMPORTANT:
    case FLARM_TRAFFIC::ALARM_URGENT:
      canvas.select(traffic_look.alarm_brush);
      break;
    case FLARM_TRAFFIC::ALARM_NONE:
      canvas.select(traffic_look.safe_brush);
      break;
    }

    // Select black pen
    canvas.black_pen();

    // Rotate and shift the arrow to the right position and angle
    PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                       traffic.track - projection.GetScreenAngle());

    // Draw the arrow
    canvas.polygon(Arrow, 5);
  }
}

/**
 * Draws the teammate icon to the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawTeammate(Canvas &canvas) const
{
  const TeamInfo &teamcode_info = Calculated();

  if (teamcode_info.teammate_available) {
    RasterPoint sc;
    if (render_projection.GeoToScreenIfVisible(teamcode_info.teammate_location,
                                                 sc))
      traffic_look.teammate_icon.draw(canvas, sc);
  }
}
