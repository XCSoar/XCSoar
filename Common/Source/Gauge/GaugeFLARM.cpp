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

#include "Gauge/GaugeFLARM.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Screen.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "NMEA/Info.h"
#include "Units.hpp"
#include "resource.h"

#include <algorithm>

using std::min;
using std::max;

#define FLARMMAXRANGE 2000

/**
 * Returns the distance scaled at a quadratic(?) scale
 * @param d Distance to the own plane
 */
int GaugeFLARM::RangeScale(double d) {
  double drad = max(0.0,1.0-d/FLARMMAXRANGE);
  return iround(radius*(1.0-drad*drad));
}

/**
 * Draws the FLARM gauge background bitmap to the given canvas
 * @param canvas Canvas for painting
 */
void GaugeFLARM::RenderBg(Canvas &canvas) {
  // Load the background bitmap
  BitmapCanvas hdcTemp(canvas, hRoseBitMap);

  // If it doesn't fit, make it fit
  if ((unsigned)hRoseBitMapSize.cx != canvas.get_width() ||
      (unsigned)hRoseBitMapSize.cy != canvas.get_height()) {
    canvas.stretch(0, 0,
                   canvas.get_width(),
                   canvas.get_height(),
                   hdcTemp,
                   0, 0, hRoseBitMapSize.cx, hRoseBitMapSize.cy);
  } else {
    canvas.copy(0, 0, canvas.get_width(), canvas.get_height(),
                hdcTemp, 0, 0);
  }
}

/**
 * Renders the FLARM traffic to the given canvas
 * @param canvas Canvas for drawing
 * @param gps_info NMEA_INFO struct containing the FLARM targets
 */
void GaugeFLARM::RenderTraffic(Canvas &canvas, const NMEA_INFO &gps_info)
{
  // TODO enhancement: support red/green Color blind pilots

  // Set font and colors
  canvas.select(TitleWindowFont);
  canvas.set_text_color(Color::BLACK);
  canvas.set_background_color(Color::WHITE);

  // Cycle through FLARM targets
  for (int i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (gps_info.FLARM_Traffic[i].ID>0) {
      // Set the arrow color depending on alarm level
      switch (gps_info.FLARM_Traffic[i].AlarmLevel) {
      case 1:
        canvas.select(MapGfx.yellowBrush);
        break;
      case 2:
      case 3:
        canvas.select(MapGfx.redBrush);
        break;
      case 0:
      case 4:
        canvas.select(MapGfx.greenBrush);
        break;
      }

      double x, y;
      x = gps_info.FLARM_Traffic[i].RelativeEast;
      y = -gps_info.FLARM_Traffic[i].RelativeNorth;
      double d = sqrt(x*x+y*y);
      if (d>0) {
        x /= d;
        y /= d;
      } else {
        x = 0;
        y = 0;
      }
      double dh = gps_info.FLARM_Traffic[i].RelativeAltitude;
      double slope = atan2(dh,d)*2.0/M_PI; // (-1,1)

      slope = max(-1.0,min(1.0,slope*2)); // scale so 45 degrees or more=90

      // QUESTION TB: what about north up mode???
      double DisplayAngle = -gps_info.TrackBearing;
      rotate(x, y, DisplayAngle); 	// or use .Heading?

      double scale = RangeScale(d);

      // Calculate screen coordinates
      POINT sc;
      sc.x = center.x + iround(x*scale);
      sc.y = center.y + iround(y*scale);

      if (gps_info.FLARM_Traffic[i].AlarmLevel>0) {
        // Draw line through target
        canvas.line(sc.x, sc.y,
                    center.x + iround(radius*x),
                    center.y + iround(radius*y));
      }

      // Create an arrow polygon
      POINT Arrow[5];
      Arrow[0].x = -3;
      Arrow[0].y = 4;
      Arrow[1].x = 0;
      Arrow[1].y = -5;
      Arrow[2].x = 3;
      Arrow[2].y = 4;
      Arrow[3].x = 0;
      Arrow[3].y = 1;
      Arrow[4].x = -3;
      Arrow[4].y = 4;

      // Rotate and shift the arrow
      PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                         gps_info.FLARM_Traffic[i].TrackBearing
                         + DisplayAngle);

      // Draw the polygon
      canvas.polygon(Arrow, 5);

      short relalt =
          iround(gps_info.FLARM_Traffic[i].RelativeAltitude*ALTITUDEMODIFY/100);

      // if (relative altitude is other than zero)
      if (relalt != 0) {
        // Write the relativ altitude devided by 100 to the Buffer
        TCHAR Buffer[10];
        _stprintf(Buffer, TEXT("%d"), abs(relalt));

        // Calculate size of the output string
        SIZE tsize = canvas.text_size(Buffer);
        tsize.cx = (tsize.cx+IBLSCALE(6))/2;

        // Draw string
        canvas.text(sc.x - tsize.cx + IBLSCALE(7),
            sc.y - tsize.cy - IBLSCALE(5),
            Buffer);

        // Set black brush for the up/down arrow
        canvas.black_brush();

        // Prepare the triangular polygon
        POINT triangle[4];
        triangle[0].x = 3;  // was  2
        triangle[0].y = -3; // was -2
        triangle[1].x = 6;  // was 4
        triangle[1].y = 1;
        triangle[2].x = 0;
        triangle[2].y = 1;

        // Flip = -1 for arrow pointing downwards
        short flip = 1;
        if (relalt<0) {
          flip = -1;
        }

        // Shift the arrow to the right position
        for (int j=0; j<3; j++) {
          triangle[j].x = sc.x+IBLSCALE(triangle[j].x)-tsize.cx;
          triangle[j].y = sc.y+flip*IBLSCALE(triangle[j].y)
          -tsize.cy/2-IBLSCALE(5);
        }
        triangle[3].x = triangle[0].x;
        triangle[3].y = triangle[0].y;

        // Draw the arrow
        canvas.polygon(triangle, 4);
      }
    }
  }
}

/**
 * Render the FLARM gauge to the buffer canvas
 * @param gps_info The NMEA_INFO struct containing the FLARM targets
 */
void GaugeFLARM::Render(const NMEA_INFO &gps_info)
{
  if (Visible) {
    // Render the background
    RenderBg(get_canvas());

    // Render the traffic on top
    RenderTraffic(get_canvas(), gps_info);

    // Draw buffer to the screen
    invalidate();
  }
}

/**
 * Constructor of the GaugeFLARM class
 * @param parent Parent window
 */
GaugeFLARM::GaugeFLARM(ContainerWindow &parent,
                       int left, int top, unsigned width, unsigned height)
  :Visible(false), ForceVisible(false), Suppress(false), Traffic(false)
{
  // start of new code for displaying FLARM window

  set(parent, left, top, width, height,
      false, false, false);

  center.x = get_hmiddle();
  center.y = get_vmiddle();
  radius = min(get_right() - center.x, get_bottom() - center.y);

  // Load the background bitmap
  hRoseBitMap.load(IDB_FLARMROSE);

  // Save the size of the background bitmap
  hRoseBitMapSize = hRoseBitMap.get_size();

  // Render Background for the first time
  RenderBg(get_canvas());

  // Hide the gauge
  Show(false);
}

/**
 * Sets the Traffic field of the class to present
 * @param present New value for the Traffic field
 */
void GaugeFLARM::TrafficPresent(bool present) {
  Traffic = present;
}

/**
 * Shows or hides the FLARM gauge depending on enable_gauge
 * @param enable_gauge Enables the gauge if true, disables otherwise
 */
void GaugeFLARM::Show(const bool enable_gauge) {
  Visible = ForceVisible || (Traffic && enable_gauge && !Suppress);
  static bool lastvisible = true;
  if (Visible && !lastvisible) {
    show();
  }
  if (!Visible && lastvisible) {
    hide();
  }
  lastvisible = Visible;
}
