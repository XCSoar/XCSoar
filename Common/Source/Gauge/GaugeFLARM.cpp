/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "XCSoar.h"
#include "Protection.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Screen.hpp"
#include "Compatibility/string.h"
#include "InfoBoxLayout.h"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Screen/PaintCanvas.hpp"
#include "options.h" /* for IBLSCALE() */
#include <stdlib.h>
#include "SettingsUser.hpp"

bool  EnableFLARMGauge = true;
DWORD EnableFLARMMap = 1;

static Color colTextGray;
static Color colText;
static Color colTextBackgnd;

#define FLARMMAXRANGE 2000

int GaugeFLARM::RangeScale(double d) {
  double drad = max(0.0,1.0-d/FLARMMAXRANGE);
  return iround(radius*(1.0-drad*drad));
}

void GaugeFLARM::RenderBg(Canvas &canvas) {
  BitmapCanvas hdcTemp(canvas, hRoseBitMap);

  if (hRoseBitMapSize.cx != IBLSCALE(InfoBoxLayout::ControlWidth * 2) ||
      hRoseBitMapSize.cy != IBLSCALE(InfoBoxLayout::ControlHeight * 2 - 1)) {
    canvas.stretch(0, 0,
                   InfoBoxLayout::ControlWidth * 2,
                   InfoBoxLayout::ControlHeight * 2 - 1,
                   hdcTemp,
                   0, 0, hRoseBitMapSize.cx, hRoseBitMapSize.cy);
  }
  else
  {
    canvas.copy(0, 0, InfoBoxLayout::ControlWidth * 2,
                InfoBoxLayout::ControlHeight * 2 - 1,
                hdcTemp, 0, 0);
  }

}

#include "WindowControls.h" // just to get colors

void GaugeFLARM::RenderTraffic(Canvas &canvas, const NMEA_INFO *gps_info)
{
  // TODO enhancement: support red/green Color blind pilots

  canvas.select(TitleWindowFont);
  canvas.set_text_color(Color(0x0,0x0,0x0));
  canvas.set_background_color(Color(0xff,0xff,0xff));

  for (int i=0; i<FLARM_MAX_TRAFFIC; i++) {
    if (gps_info->FLARM_Traffic[i].ID>0) {

      switch (gps_info->FLARM_Traffic[i].AlarmLevel) {
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
      x = gps_info->FLARM_Traffic[i].RelativeEast;
      y = -gps_info->FLARM_Traffic[i].RelativeNorth;
      double d = sqrt(x*x+y*y);
      if (d>0) {
	x/= d;
	y/= d;
      } else {
	x= 0;
	y= 0;
      }
      double dh = gps_info->FLARM_Traffic[i].RelativeAltitude;
      double slope = atan2(dh,d)*2.0/3.14159; // (-1,1)

      slope = max(-1.0,min(1.0,slope*2)); // scale so 45 degrees or more=90

      double DisplayAngle = -gps_info->TrackBearing;
      rotate(x, y, DisplayAngle); 	// or use .Heading?
      double scale = RangeScale(d);

      POINT sc;
      sc.x = center.x + iround(x*scale);
      sc.y = center.y + iround(y*scale);

      if (gps_info->FLARM_Traffic[i].AlarmLevel>0) {
        // Draw line through target
        canvas.line(sc.x, sc.y,
                    center.x + iround(radius*x),
                    center.y + iround(radius*y));
      }

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

      PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                         gps_info->FLARM_Traffic[i].TrackBearing
                         + DisplayAngle);
      canvas.polygon(Arrow, 5);

      short relalt =
	iround(gps_info->FLARM_Traffic[i].RelativeAltitude*ALTITUDEMODIFY/100);

      if (relalt != 0) {
	TCHAR Buffer[10];
	_stprintf(Buffer, TEXT("%d"), abs(relalt));
        SIZE tsize = canvas.text_size(Buffer);
	tsize.cx = (tsize.cx+IBLSCALE(6))/2;
        canvas.text(sc.x - tsize.cx + IBLSCALE(7),
                    sc.y - tsize.cy - IBLSCALE(5),
                    Buffer);
        canvas.black_brush();
	POINT triangle[4];
	triangle[0].x = 3;  // was  2
	triangle[0].y = -3; // was -2
	triangle[1].x = 6;  // was 4
	triangle[1].y = 1;
	triangle[2].x = 0;
	triangle[2].y = 1;
	short flip = 1;
	if (relalt<0) {
	  flip = -1;
	}
	for (int j=0; j<3; j++) {
	  triangle[j].x = sc.x+IBLSCALE(triangle[j].x)-tsize.cx;
	  triangle[j].y = sc.y+flip*IBLSCALE(triangle[j].y)
	    -tsize.cy/2-IBLSCALE(5);
          }
	triangle[3].x = triangle[0].x;
	triangle[3].y = triangle[0].y;
        canvas.polygon(triangle, 4);

      }
    }
  }
}


void GaugeFLARM::Render(const NMEA_INFO *gps_info)
{
  if (Visible) {
    RenderBg(get_canvas());
    RenderTraffic(get_canvas(), gps_info);

    commit_buffer();
  }
}


GaugeFLARM::GaugeFLARM(ContainerWindow &parent)
  :Visible(false), ForceVisible(false), Suppress(false), Traffic(false)
{
  // start of new code for displaying FLARM window

  RECT rc = parent.get_client_rect();

  set(parent,
      (int)(rc.right - InfoBoxLayout::ControlWidth * 2)+1,
      (int)(rc.bottom - InfoBoxLayout::ControlHeight * 2)+1,
      (int)(InfoBoxLayout::ControlWidth * 2)-1,
      (int)(InfoBoxLayout::ControlHeight * 2)-1,
      false, false, false);
  insert_after(HWND_TOP, false);

  rc = get_client_rect();

  center.x = get_hmiddle();
  center.y = get_vmiddle();
  radius = min(get_right() - center.x, get_bottom() - center.y);

  hRoseBitMap.load(IDB_FLARMROSE);

  hRoseBitMapSize = hRoseBitMap.get_size();

  if (Appearance.InverseInfoBox){
    colText = Color(0xff, 0xff, 0xff);
    colTextBackgnd = Color(0x00, 0x00, 0x00);
    colTextGray = Color(0xa0, 0xa0, 0xa0);
  } else {
    colText = Color(0x00, 0x00, 0x00);
    colTextBackgnd = Color(0xff, 0xff, 0xff);
    colTextGray = Color(~0xa0, ~0xa0, ~0xa0);
  }

  get_canvas().set_text_color(colText);
  get_canvas().set_background_color(colTextBackgnd);

  // end of new code for drawing FLARM window (see below for destruction of objects)

  // turn off suppression
  Suppress = false;

  install_wndproc();
  hide();

  RenderBg(get_canvas());
  commit_buffer();

  Visible = false;
  Traffic = false;
  Show();
}


void GaugeFLARM::TrafficPresent(bool present) {
  Traffic = present;
}


void GaugeFLARM::Show() {
  Visible = ForceVisible || (Traffic && EnableFLARMGauge && !Suppress);
  static bool lastvisible = true;
  if (Visible && !lastvisible) {
    show();
  }
  if (!Visible && lastvisible) {
    hide();
  }
  lastvisible = Visible;
}
