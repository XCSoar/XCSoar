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

#include "StdAfx.h"
#include "GaugeVario.h"
#include "MapWindow.h"
#include "Utils.h"
#include "externs.h"
#include "InfoBoxLayout.h"

#include "widget.hxx"

static Widget vario_widget;
static BufferCanvas vario_buffer;
HWND hWndVarioWindow = NULL; // Vario Window
extern HFONT CDIWindowFont; // New
extern HWND hWndMenuButton;

extern HFONT InfoWindowFont;

#define GAUGEXSIZE (InfoBoxLayout::ControlWidth)
#define GAUGEYSIZE (InfoBoxLayout::ControlHeight*3)

void GaugeVario::Create() {
  RECT bigrc;

  bigrc = MapWindow::MapRect;

  vario_widget.set(hWndMainWindow,
                   bigrc.right + InfoBoxLayout::ControlWidth,
                   bigrc.top,
                   GAUGEXSIZE, GAUGEYSIZE);
  vario_widget.insert_after(hWndMenuButton);
  vario_buffer.set(vario_widget, GAUGEXSIZE, GAUGEYSIZE);

  hWndVarioWindow = vario_widget;

  SendMessage(hWndVarioWindow,WM_SETFONT,
              (WPARAM)CDIWindowFont,MAKELPARAM(TRUE,0));
}


void GaugeVario::Destroy() {
  vario_buffer.reset();
  vario_widget.reset();
}

#define GAUGEVARIORANGE 2.50 // 5 m/s
#define GAUGEVARIOSWEEP 90 // degrees total sweep

extern NMEA_INFO     GPS_INFO;
extern DERIVED_INFO  CALCULATED_INFO;

//extern DERIVED_INFO DerivedDrawInfo;
//extern NMEA_INFO DrawInfo;

void GaugeVario::Render() {
  RenderBg();

  vario_buffer.black_brush();
  vario_buffer.black_pen();

  TCHAR Temp[10];

  // draw dashes
  POINT bit[3];
  int i;
  int xoffset = 80;
  int yoffset = vario_buffer.get_height() / 2;
  int degrees_per_unit = (int)((GAUGEVARIOSWEEP/2.0)/(GAUGEVARIORANGE*LIFTMODIFY));
  int gmax = (int)(degrees_per_unit*(GAUGEVARIORANGE*LIFTMODIFY))+2;
  double dx, dy;
  double vval;

  if (GPS_INFO.VarioAvailable) {
    vval = GPS_INFO.Vario;
  } else {
    vval = CALCULATED_INFO.Vario;
  }
  i = (int)(vval*degrees_per_unit*LIFTMODIFY);
  i = min(gmax,max(-gmax,i));

  dx = -xoffset+10; dy = 4;
  rotate(dx, dy, i);
  bit[0].x = (int)(dx+xoffset); bit[0].y = (int)(dy+yoffset);

  dx = -xoffset+10; dy = -4;
  rotate(dx, dy, i);
  bit[1].x = (int)(dx+xoffset); bit[1].y = (int)(dy+yoffset);

  dx = -xoffset+3; dy = 0;
  rotate(dx, dy, i);
  bit[2].x = (int)(dx+xoffset); bit[2].y = (int)(dy+yoffset);

  _stprintf(Temp,TEXT("%2.1f"), vval*LIFTMODIFY);
  vario_buffer.font(InfoWindowFont);
  vario_buffer.bottom_right_text(vario_buffer.get_width() - 2,
                                 vario_buffer.get_height() - 2,
                                 Temp);

  vario_buffer.line(bit[0].x, bit[0].y, bit[1].x, bit[1].y);
  vario_buffer.line(bit[1].x, bit[1].y, bit[2].x, bit[2].y);

  vario_widget.get_canvas().copy(vario_buffer);
}


void GaugeVario::RenderBg() {
  vario_buffer.white_pen();
  vario_buffer.white_brush();
  vario_buffer.clear();

  vario_buffer.black_pen();

  // draw dashes
  POINT bit[4];
  int i;
  int xoffset = 80;
  int yoffset = vario_buffer.get_height() / 2;
  int degrees_per_unit =
    (int)((GAUGEVARIOSWEEP/2.0)/(GAUGEVARIORANGE*LIFTMODIFY));
  int gmax =
    (int)(degrees_per_unit*(GAUGEVARIORANGE*LIFTMODIFY+1));
  double dx, dy;
  for (i= 0; i<= gmax; i+= degrees_per_unit) {
    if (i==0) {
      dx = -xoffset+15; dy = 0;
    } else {
      dx = -xoffset+5; dy = 0;
    }
    rotate(dx, dy, i);
    bit[0].x = (int)(dx+xoffset); bit[0].y = (int)(dy+yoffset);

    dx = -xoffset; dy = 0; rotate(dx, dy, i);
    bit[1].x = (int)(dx+xoffset); bit[1].y = (int)(dy+yoffset);

    vario_buffer.line(bit[0].x, bit[0].y, bit[1].x, bit[1].y);

    bit[2] = bit[1];
    if (i>0) {
      vario_buffer.line(bit[2].x, bit[2].y, bit[3].x, bit[3].y);
    }
    bit[3] = bit[1];
  }
  for (i= 0; i>= -gmax; i-= degrees_per_unit) {
    if (i==0) {
      dx = -xoffset+15; dy = 0;
    } else {
      dx = -xoffset+5; dy = 0;
    }
    rotate(dx, dy, i);
    bit[0].x = (int)(dx+xoffset); bit[0].y = (int)(dy+yoffset);

    dx = -xoffset; dy = 0; rotate(dx, dy, i);
    bit[1].x = (int)(dx+xoffset); bit[1].y = (int)(dy+yoffset);

    vario_buffer.line(bit[0].x, bit[0].y, bit[1].x, bit[1].y);

    bit[2] = bit[1];
    if (i<0) {
      vario_buffer.line(bit[2].x, bit[2].y, bit[3].x, bit[3].y);
    }
    bit[3] = bit[1];
  }

}
