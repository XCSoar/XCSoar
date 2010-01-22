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

#include "Task/TaskManager.hpp"
#include "MapWindow.h"
#include "Math/Screen.hpp"
#include "Math/Earth.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "StringUtil.hpp"

#define fSnailColour(cv) max((short)0, min((short)(NUMSNAILCOLORS-1), (short)((cv+1.0)/2.0*NUMSNAILCOLORS)))

/**
 * Draws the FLARM traffic icons onto the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawFLARMTraffic(Canvas &canvas)
{
  // Return if FLARM icons on moving map are disabled
  if (!SettingsMap().EnableFLARMMap) return;

  // Return if FLARM data is not available
  const FLARM_STATE &flarm = Basic().flarm;
  if (!flarm.FLARM_Available)
    return;

  // Create pen for icon outlines
  Pen thinBlackPen(IBLSCALE(1), Color(0, 0, 0));
  canvas.select(thinBlackPen);

  // Create point array that will form that arrow polygon
  POINT Arrow[5];

  // double dX, dY;
  TextInBoxMode_t displaymode;
  displaymode.AsInt = 0;

  // Determine scale factor for use in Scaled mode
  double screenrange = GetScreenDistanceMeters();
  double scalefact = screenrange/6000.0;

  // Create the brushes for filling the arrow (red/yellow/green)
  static const Brush AlarmBrush(Color::RED);
  static const Brush WarningBrush(Color(0xFF, 0xA2, 0x00));
  static const Brush TrafficBrush(Color::GREEN);

  // Saves the McCready value
  const double MACCREADY = task != NULL
    ? task->get_glide_polar().get_mc() : fixed_zero;

  // Circle through the FLARM targets
  for (unsigned i = 0; i < FLARM_STATE::FLARM_MAX_TRAFFIC; i++) {
    const FLARM_TRAFFIC &traffic = flarm.FLARM_Traffic[i];

    // if FLARM target i exists
    if (!traffic.defined())
      continue;

    // Save the location of the FLARM target
    GEOPOINT target_loc = traffic.Location;

    // If Scaled mode is chosen, recalculate the
    // targets virtual position using the scale factor
    if ((SettingsMap().EnableFLARMMap == 2) && (scalefact > 1.0)) {
      fixed distance;
      fixed bearing;

      DistanceBearing(Basic().Location, target_loc, &distance, &bearing);

      FindLatitudeLongitude(Basic().Location, bearing, distance * scalefact,
                            &target_loc);
    }

    // TODO feature: draw direction, rel height?

    // Points for the screen coordinates for the icon, name and average climb
    POINT sc, sc_name, sc_av;

    // If FLARM target not on the screen, move to the next one
    if (!LonLat2ScreenIfVisible(target_loc, &sc))
      continue;

    // Draw the name 16 points below the icon
    sc_name = sc;
    sc_name.y -= IBLSCALE(16);

    // Draw the average climb value above the icon
    sc_av = sc;
    sc_av.y += IBLSCALE(16);

#ifndef FLARM_AVERAGE
    if (traffic.HasName())
      TextInBox(hDC, traffic.Name, sc.x + IBLSCALE(3), sc.y, 0, displaymode, true);
#else
    TCHAR label_name[100];
    TCHAR label_avg[100];

    sc_av.x += IBLSCALE(3);

    if (traffic.HasName()) {
      sc_name.y -= IBLSCALE(8);
      _stprintf(label_name, TEXT("%s"), traffic.Name);
    } else {
      label_name[0] = _T('\0');
    }

    if (traffic.Average30s >= 0.1) {
      _stprintf(label_avg, TEXT("%.1f"), LIFTMODIFY * traffic.Average30s);
    } else {
      label_avg[0] = _T('\0');
    }

#ifndef NDEBUG
    // for testing only!
    _stprintf(label_avg, TEXT("2.3"));
    _stprintf(label_name, TEXT("WUE"));
#endif

    // JMW TODO enhancement: decluttering of FLARM altitudes (sort by max lift)

    int dx = (sc_av.x - Orig_Aircraft.x);
    int dy = (sc_av.y - Orig_Aircraft.y);

    // only draw labels if not close to aircraft
    if (dx * dx + dy * dy > IBLSCALE(30) * IBLSCALE(30)) {
      // Select the MapLabelFont and black color
      canvas.select(MapLabelFont);
      canvas.set_text_color(Color(0, 0, 0));

      // If FLARM callsign/name available draw it to the canvas
      if (!string_is_empty(label_name))
        canvas.text_opaque(sc_name.x, sc_name.y, label_name);

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
        float vmax = (float)(1.5 * min(5.0, max(MACCREADY, 0.5)));
        float vmin = (float)(-1.5 * min(5.0, max(MACCREADY, 2.0)));

        float cv = traffic.Average30s;
        if (cv < 0)
          cv /= (-vmin); // JMW fixed bug here
        else
          cv /= vmax;

        int colourIndex = fSnailColour(cv);
        // Select the appropriate background color determined before
        canvas.select(MapGfx.hSnailPens[colourIndex]);
        Brush hVarioBrush(MapGfx.hSnailColours[colourIndex]);
        canvas.select(hVarioBrush);

        // Draw the rounded background rectangle
        canvas.round_rectangle(brect.left, brect.top,
                               brect.right, brect.bottom,
                               IBLSCALE(8), IBLSCALE(8));

#ifdef WINDOWSPC
        canvas.background_transparent();
        canvas.text(sc_av.x, sc_av.y, label_avg);
#else
        // Draw the average climb value on top
        canvas.text_opaque(sc_av.x, sc_av.y, label_avg);
#endif
      }
    }
#endif

    // If FLARM alarm draw alarm icon below corresponding target
    if ((traffic.AlarmLevel > 0) && (traffic.AlarmLevel < 4)) {
      draw_masked_bitmap(canvas, MapGfx.hFLARMTraffic, sc.x, sc.y, 10, 10, true);
    }

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

    // double vmag = max(1.0,min(15.0,traffic.Speed/5.0))*2;

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
                       traffic.TrackBearing - GetDisplayAngle());

    // Draw the arrow
    canvas.polygon(Arrow, 5);
  }
}

/**
 * Draws the teammate icon to the given canvas
 * @param canvas Canvas for drawing
 */
void
MapWindow::DrawTeammate(Canvas &canvas)
{
  if (SettingsComputer().TeammateCodeValid) {
    draw_masked_bitmap_if_visible(canvas, MapGfx.hBmpTeammatePosition,
        Calculated().TeammateLocation, 20, 20);
  }
}
