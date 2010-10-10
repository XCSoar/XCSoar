/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "StringUtil.hpp"
#include "GlideSolvers/GlidePolar.hpp"

#include <stdio.h>

gcc_const
static int
fSnailColour(fixed cv)
{
  return max((short)0, min((short)(NUMSNAILCOLORS - 1),
                           (short)((cv + fixed_one) / 2 * NUMSNAILCOLORS)));
}

/**
 * Draws the FLARM traffic icons onto the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawFLARMTraffic(Canvas &canvas) const
{
  // Return if FLARM icons on moving map are disabled
  if (!SettingsMap().EnableFLARMMap)
    return;

  // Return if FLARM data is not available
  const FLARM_STATE &flarm = Basic().flarm;
  if (!flarm.FLARM_Available)
    return;

  const MapWindowProjection &projection = render_projection;

  // Create pen for icon outlines
  Pen thinBlackPen(IBLSCALE(1), Color(0, 0, 0));
  canvas.select(thinBlackPen);

  // Create point array that will form that arrow polygon
  POINT Arrow[5];

  // Determine scale factor for use in Scaled mode
  fixed screenrange = projection.GetScreenDistanceMeters();
  fixed scalefact = screenrange / 6000;

  // Create the brushes for filling the arrow (red/yellow/green)
  static const Brush AlarmBrush(Color::RED);
  static const Brush WarningBrush(Color(0xFF, 0xA2, 0x00));
  static const Brush TrafficBrush(Color::GREEN);

  // Saves the McCready value
  const fixed MACCREADY = get_glide_polar().get_mc();

  // Circle through the FLARM targets
  for (unsigned i = 0; i < FLARM_STATE::FLARM_MAX_TRAFFIC; i++) {
    const FLARM_TRAFFIC &traffic = flarm.FLARM_Traffic[i];

    // if FLARM target i exists
    if (!traffic.defined())
      continue;

    // Save the location of the FLARM target
    GeoPoint target_loc = traffic.Location;

    // If Scaled mode is chosen, recalculate the
    // targets virtual position using the scale factor
    if ((SettingsMap().EnableFLARMMap == 2) && (scalefact > fixed_one)) {
      fixed distance;
      Angle bearing;

      DistanceBearing(Basic().Location, target_loc, &distance, &bearing);

      FindLatitudeLongitude(Basic().Location, bearing,
                            distance * scalefact, &target_loc);
    }

    // TODO feature: draw direction, rel height?

    // Points for the screen coordinates for the icon, name and average climb
    POINT sc, sc_name, sc_av;

    // If FLARM target not on the screen, move to the next one
    if (!projection.LonLat2ScreenIfVisible(target_loc, &sc))
      continue;

    // Draw the name 16 points below the icon
    sc_name = sc;
    sc_name.y -= IBLSCALE(16);

    // Draw the average climb value above the icon
    sc_av = sc;
    sc_av.y += IBLSCALE(16);

    const TCHAR *label_name;
    TCHAR label_avg[100];

    sc_av.x += IBLSCALE(3);

    if (traffic.HasName()) {
      sc_name.y -= IBLSCALE(8);
      label_name = traffic.Name;
    } else {
      label_name = NULL;
    }

    if (traffic.Average30s >= fixed(0.1))
      Units::FormatUserVSpeed(traffic.Average30s, label_avg, 100, false);
    else
      label_avg[0] = _T('\0');

    // JMW TODO enhancement: decluttering of FLARM altitudes (sort by max lift)

    int dx = sc_av.x - projection.GetOrigAircraft().x;
    int dy = sc_av.y - projection.GetOrigAircraft().y;

    // only draw labels if not close to aircraft
    if (dx * dx + dy * dy > IBLSCALE(30) * IBLSCALE(30)) {
      // Select the MapLabelFont and black color
      canvas.select(Fonts::MapLabel);
      canvas.set_text_color(Color(0, 0, 0));

      // If FLARM callsign/name available draw it to the canvas
      if (label_name != NULL && !string_is_empty(label_name)) {
        canvas.set_background_color(Color::WHITE);
        canvas.text(sc_name.x, sc_name.y, label_name);
      }

      // If average climb data available draw it to the canvas
      if (!string_is_empty(label_avg)) {
        SIZE tsize;
        RECT brect;

        // Calculate the size of the average climb indicator
        tsize = canvas.text_size(label_avg);
        brect.left = sc_av.x - 2;
        brect.right = brect.left + tsize.cx + 6;
        brect.top = sc_av.y + ((tsize.cy + 4) >> 3) - 2;
        brect.bottom = brect.top + 3 + tsize.cy - ((tsize.cy + 4) >> 3);

        // Determine the background color for the average climb indicator
        fixed vmax = (fixed(1.5) * min(fixed(5), max(MACCREADY, fixed_half)));
        fixed vmin = (fixed(-1.5) * min(fixed(5), max(MACCREADY, fixed_two)));

        fixed cv(traffic.Average30s);
        if (negative(cv))
          cv /= (-vmin); // JMW fixed bug here
        else
          cv /= vmax;

        int colourIndex = fSnailColour(cv);
        // Select the appropriate background color determined before
        canvas.select(Graphics::hSnailPens[colourIndex]);
        Brush hVarioBrush(Graphics::hSnailColours[colourIndex]);
        canvas.select(hVarioBrush);

        // Draw the rounded background rectangle
        canvas.round_rectangle(brect.left, brect.top,
                               brect.right, brect.bottom,
                               IBLSCALE(8), IBLSCALE(8));

        canvas.background_transparent();
        canvas.text(sc_av.x, sc_av.y, label_avg);
      }
    }

    // If FLARM alarm draw alarm icon below corresponding target
    if ((traffic.AlarmLevel > 0) && (traffic.AlarmLevel < 4))
      Graphics::hFLARMTraffic.draw(canvas, bitmap_canvas, sc.x, sc.y);

    // Fill the Arrow array with a normal arrow pointing north
    Arrow[0].x = -4;
    Arrow[0].y = 5;
    Arrow[1].x = 0;
    Arrow[1].y = -6;
    Arrow[2].x = 4;
    Arrow[2].y = 5;
    Arrow[3].x = 0;
    Arrow[3].y = 2;
    Arrow[4].x = -4;
    Arrow[4].y = 5;

    // Select brush depending on AlarmLevel
    switch (traffic.AlarmLevel) {
    case 1:
      canvas.select(WarningBrush);
      break;
    case 2:
    case 3:
      canvas.select(AlarmBrush);
      break;
    case 0:
    case 4:
      canvas.select(TrafficBrush);
      break;
    }

    // Select black pen
    static Pen BlackPen(IBLSCALE(1), Color::BLACK);
    canvas.select(BlackPen);

    // Rotate and shift the arrow to the right position and angle
    PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                       traffic.TrackBearing - projection.GetDisplayAngle());

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
  if (SettingsComputer().TeammateCodeValid) {
    POINT sc;
    if (render_projection.LonLat2ScreenIfVisible(Calculated().TeammateLocation,
                                                 &sc))
      Graphics::hBmpTeammatePosition.draw(canvas, bitmap_canvas, sc.x, sc.y);
  }
}
