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

#include "Screen/Chart.hpp"
#include "XCSoar.h"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/MainWindow.hpp"
#include "InfoBoxLayout.h"
#include "Language.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"


#define BORDER_X 24
#define BORDER_Y 19

const COLORREF Chart::GROUND_COLOUR = RGB(157,101,60);

void Chart::ResetScale() {
  unscaled_y = true;
  unscaled_x = true;
  xscale = 0.0;
  yscale = 0.0;
  x_min = 0;
  x_max = 0;
  y_min = 0;
  y_max = 0;
}

Chart::Chart(const Canvas the_canvas, const RECT the_rc)
  :canvas(the_canvas),rc(the_rc) {

  ResetScale();
}

void Chart::ScaleYFromData(LeastSquares* lsdata)
{
  if (!lsdata->sum_n) {
    return;
  }

  if (unscaled_y) {
    y_min = lsdata->y_min;
    y_max = lsdata->y_max;
    unscaled_y = false;
  } else {
    y_min = min(y_min,lsdata->y_min);
    y_max = max(y_max,lsdata->y_max);
  }

  if (lsdata->sum_n>1) {
    double y0, y1;
    y0 = lsdata->x_min*lsdata->m+lsdata->b;
    y1 = lsdata->x_max*lsdata->m+lsdata->b;
    y_min = min(y_min,min(y0,y1));
    y_max = max(y_max,max(y0,y1));
  }


  if (fabs(y_max - y_min) > 50){
    yscale = (y_max - y_min);
    if (yscale>0) {
      yscale = (rc.bottom-rc.top-BORDER_Y)/yscale;
    }
  } else {
    yscale = 2000;
  }
}


void Chart::ScaleXFromData(LeastSquares* lsdata)
{
  if (!lsdata->sum_n) {
    return;
  }
  if (unscaled_x) {
    x_min = lsdata->x_min;
    x_max = lsdata->x_max;
    unscaled_x = false;
  } else {
    x_min = min(x_min,lsdata->x_min);
    x_max = max(x_max,lsdata->x_max);
  }

  xscale = (x_max-x_min);
  if (xscale>0) {
    xscale = (rc.right-rc.left-BORDER_X)/xscale;
  }
}


void Chart::ScaleYFromValue(const double value)
{
  if (unscaled_y) {
    y_min = value;
    y_max = value;
    unscaled_y = false;
  } else {
    y_min = min(value, y_min);
    y_max = max(value, y_max);
  }

  yscale = (y_max - y_min);
  if (yscale>0) {
    yscale = (rc.bottom-rc.top-BORDER_Y)/yscale;
  }
}


void Chart::ScaleXFromValue(const double value)
{
  if (unscaled_x) {
    x_min = value;
    x_max = value;
    unscaled_x = false;
  } else {
    x_min = min(value, x_min);
    x_max = max(value, x_max);
  }

  xscale = (x_max-x_min);
  if (xscale>0) {
    xscale = (rc.right-rc.left-BORDER_X)/xscale;
  }

}


void Chart::StyleLine(const POINT l1, const POINT l2,
		      const int Style) {
  int minwidth = 1;
#ifndef GNAV
  minwidth = 2;
#endif
  Pen penThinSignal;
  POINT line[2];
  line[0] = l1;
  line[1] = l2;
  switch (Style) {
  case STYLE_BLUETHIN:
    canvas.clipped_dashed_line(
		 minwidth,
		 l1,
		 l2,
		 RGB(0,50,255), rc);
    break;
  case STYLE_REDTHICK:
    canvas.clipped_dashed_line(3,
		 l1,
		 l2,
		 RGB(200,50,50), rc);
    break;
  case STYLE_DASHGREEN:
    canvas.clipped_dashed_line(2,
		 line[0],
		 line[1],
		 RGB(0,255,0), rc);
    break;
  case STYLE_MEDIUMBLACK:
#ifndef GNAV
    penThinSignal.set(2, Color(50, 243, 45));
#else
    penThinSignal.set(1, Color(50, 243, 45));
#endif
    canvas.select(penThinSignal);
    canvas.clipped_polyline(line, 2, rc);
    break;
  case STYLE_THINDASHPAPER:
    canvas.clipped_dashed_line(1,
		 l1,
		 l2,
		 RGB(0x60,0x60,0x60), rc);
    break;

  default:
    break;
  }

}


void Chart::DrawLabel(const TCHAR *text,
		      const double xv, const double yv) {

  SIZE tsize;
  GetTextExtentPoint(canvas, text, _tcslen(text), &tsize);
  int x = (int)((xv-x_min)*xscale)+rc.left-tsize.cx/2+BORDER_X;
  int y = (int)((y_max-yv)*yscale)+rc.top-tsize.cy/2;
  canvas.background_opaque();
  canvas.text_opaque(x, y, &rc, text);
  canvas.background_transparent();
}


void Chart::DrawNoData() {

  SIZE tsize;
  TCHAR text[80];
  _stprintf(text,TEXT("%s"), gettext(TEXT("No data")));
  GetTextExtentPoint(canvas, text, _tcslen(text), &tsize);
  int x = (int)(rc.left+rc.right-tsize.cx)/2;
  int y = (int)(rc.top+rc.bottom-tsize.cy)/2;
  canvas.background_opaque();
  canvas.text_opaque(x, y, &rc, text);
  canvas.background_transparent();
}


void Chart::DrawXLabel(const TCHAR *text) {
  SIZE tsize;
  canvas.select(MapLabelFont);
  GetTextExtentPoint(canvas, text, _tcslen(text), &tsize);
  int x = rc.right-tsize.cx-IBLSCALE(3);
  int y = rc.bottom-tsize.cy;
  canvas.text_opaque(x, y, &rc, text);
}


void Chart::DrawYLabel(const TCHAR *text) {
  SIZE tsize;
  canvas.select(MapLabelFont);
  GetTextExtentPoint(canvas, text, _tcslen(text), &tsize);
  int x = max(2,rc.left-tsize.cx);
  int y = rc.top;
  canvas.text_opaque(x, y, &rc, text);
}


void Chart::DrawTrend(LeastSquares* lsdata, const int Style)
{
  if (lsdata->sum_n<2) {
    return;
  }

  if (unscaled_x || unscaled_y) {
    return;
  }

  double xmin, xmax, ymin, ymax;
  xmin = lsdata->x_min;
  xmax = lsdata->x_max;
  ymin = lsdata->x_min*lsdata->m+lsdata->b;
  ymax = lsdata->x_max*lsdata->m+lsdata->b;

  xmin = (int)((xmin-x_min)*xscale)+rc.left+BORDER_X;
  xmax = (int)((xmax-x_min)*xscale)+rc.left+BORDER_X;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(line[0], line[1], Style);
}


void Chart::DrawTrendN(LeastSquares* lsdata, const int Style)
{
  if (lsdata->sum_n<2) {
    return;
  }

  if (unscaled_x || unscaled_y) {
    return;
  }

  double xmin, xmax, ymin, ymax;
  xmin = 0.5;
  xmax = lsdata->sum_n+0.5;
  ymin = lsdata->x_min*lsdata->m+lsdata->b;
  ymax = lsdata->x_max*lsdata->m+lsdata->b;

  xmin = (int)((xmin)*xscale)+rc.left+BORDER_X;
  xmax = (int)((xmax)*xscale)+rc.left+BORDER_X;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(line[0], line[1], Style);

}


void Chart::DrawLine(const double xmin, const double ymin,
		     const double xmax, const double ymax,
		     const int Style) {

  if (unscaled_x || unscaled_y) {
    return;
  }
  POINT line[2];
  line[0].x = (int)((xmin-x_min)*xscale)+rc.left+BORDER_X;
  line[0].y = (int)((y_max-ymin)*yscale)+rc.top;
  line[1].x = (int)((xmax-x_min)*xscale)+rc.left+BORDER_X;
  line[1].y = (int)((y_max-ymax)*yscale)+rc.top;

  StyleLine(line[0], line[1], Style);

}


void Chart::DrawBarChart(LeastSquares* lsdata) {
  int i;

  if (unscaled_x || unscaled_y) {
    return;
  }

  SelectObject(canvas, GetStockObject(WHITE_PEN));
  SelectObject(canvas, GetStockObject(WHITE_BRUSH));

  int xmin, ymin, xmax, ymax;

  for (i= 0; i<lsdata->sum_n; i++) {
    xmin = (int)((i+1+0.2)*xscale)+rc.left+BORDER_X;
    ymin = (int)((y_max-y_min)*yscale)+rc.top;
    xmax = (int)((i+1+0.8)*xscale)+rc.left+BORDER_X;
    ymax = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    Rectangle(canvas,
              xmin,
              ymin,
              xmax,
              ymax);
  }
}


void Chart::DrawFilledLineGraph(LeastSquares* lsdata,
				const COLORREF color) {

  POINT line[4];

  for (int i=0; i<lsdata->sum_n-1; i++) {
    line[0].x = (int)((lsdata->xstore[i]-x_min)*xscale)+rc.left+BORDER_X;
    line[0].y = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    line[1].x = (int)((lsdata->xstore[i+1]-x_min)*xscale)+rc.left+BORDER_X;
    line[1].y = (int)((y_max-lsdata->ystore[i+1])*yscale)+rc.top;
    line[2].x = line[1].x;
    line[2].y = rc.bottom-BORDER_Y;
    line[3].x = line[0].x;
    line[3].y = rc.bottom-BORDER_Y;
    Polygon(canvas, line, 4);
  }
}



void Chart::DrawLineGraph(LeastSquares* lsdata,
			  int Style) {

  POINT line[2];

  for (int i=0; i<lsdata->sum_n-1; i++) {
    line[0].x = (int)((lsdata->xstore[i]-x_min)*xscale)+rc.left+BORDER_X;
    line[0].y = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    line[1].x = (int)((lsdata->xstore[i+1]-x_min)*xscale)+rc.left+BORDER_X;
    line[1].y = (int)((y_max-lsdata->ystore[i+1])*yscale)+rc.top;

    // STYLE_DASHGREEN
    // STYLE_MEDIUMBLACK
    StyleLine(line[0], line[1], Style);
  }
}


void Chart::FormatTicText(TCHAR *text, const double val, const double step) {
  if (step<1.0) {
    _stprintf(text, TEXT("%.1f"), val);
  } else {
    _stprintf(text, TEXT("%.0f"), val);
  }
}


void Chart::DrawXGrid(const double tic_step,
		      const double zero,
		      const int Style,
		      const double unit_step, bool draw_units) {

  POINT line[2];

  double xval;

  int xmin, ymin, xmax, ymax;
  if (!tic_step) return;

  //  bool do_units = ((x_max-zero)/tic_step)<10;

  for (xval=zero; xval<= x_max; xval+= tic_step) {

    xmin = (int)((xval-x_min)*xscale)+rc.left+BORDER_X;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax-BORDER_Y;

    // STYLE_THINDASHPAPER
    if ((xval< x_max)
        && (xmin>=rc.left+BORDER_X) && (xmin<=rc.right)) {
      StyleLine(line[0], line[1], Style);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, xval*unit_step/tic_step, unit_step);
	canvas.background_opaque();
	canvas.text_opaque(xmin, ymax-IBLSCALE(17), &rc, unit_text);
	canvas.background_transparent();
      }
    }

  }

  for (xval=zero-tic_step; xval>= x_min; xval-= tic_step) {

    xmin = (int)((xval-x_min)*xscale)+rc.left+BORDER_X;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax-BORDER_Y;

    // STYLE_THINDASHPAPER

    if ((xval> x_min)
        && (xmin>=rc.left+BORDER_X) && (xmin<=rc.right)) {

      StyleLine(line[0], line[1], Style);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, xval*unit_step/tic_step, unit_step);
	canvas.background_opaque();
	canvas.text_opaque(xmin, ymax-IBLSCALE(17), &rc, unit_text);
	canvas.background_transparent();
      }
    }
  }
}

void Chart::DrawYGrid(const double tic_step,
		      const double zero,
		      const int Style,
		      const double unit_step, bool draw_units) {

  POINT line[2];

  double yval;

  int xmin, ymin, xmax, ymax;

  if (!tic_step) return;

  for (yval=zero; yval<= y_max; yval+= tic_step) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin+BORDER_X;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    if ((yval< y_max) &&
        (ymin>=rc.top) && (ymin<=rc.bottom-BORDER_Y)) {

      StyleLine(line[0], line[1], Style);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, yval*unit_step/tic_step, unit_step);
	canvas.background_opaque();
	canvas.text_opaque(xmin+IBLSCALE(8), ymin, &rc, unit_text);
	canvas.background_transparent();
      }
    }
  }

  for (yval=zero-tic_step; yval>= y_min; yval-= tic_step) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin+BORDER_X;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    if ((yval> y_min) &&
        (ymin>=rc.top) && (ymin<=rc.bottom-BORDER_Y)) {

      StyleLine(line[0], line[1], Style);

      if (draw_units) {
	TCHAR unit_text[MAX_PATH];
	FormatTicText(unit_text, yval*unit_step/tic_step, unit_step);
	canvas.background_opaque();
	canvas.text_opaque(xmin+IBLSCALE(8), ymin, &rc, unit_text);
	canvas.background_transparent();
      }
    }
  }
}


void Chart::ScaleMakeSquare() {
  if (y_max-y_min<=0) return;
  if (rc.bottom-rc.top-BORDER_Y<=0) return;
  double ar = ((double)(rc.right-rc.left-BORDER_X))
    /(rc.bottom-rc.top-BORDER_Y);
  double ard = (x_max-x_min)/(y_max-y_min);
  double armod = ard/ar;
  double delta;

  if (armod<1.0) {
    // need to expand width
    delta = (x_max-x_min)*(1.0/armod-1.0);
    x_max += delta/2.0;
    x_min -= delta/2.0;
  } else {
    // need to expand height
    delta = (y_max-y_min)*(armod-1.0);
    y_max += delta/2.0;
    y_min -= delta/2.0;
  }
  // shrink both by 10%
  delta = (x_max-x_min)*(1.1-1.0);
  x_max += delta/2.0;
  x_min -= delta/2.0;
  delta = (y_max-y_min)*(1.1-1.0);
  y_max += delta/2.0;
  y_min -= delta/2.0;

  yscale = (rc.bottom-rc.top-BORDER_Y)/(y_max-y_min);
  xscale = (rc.right-rc.left-BORDER_X)/(x_max-x_min);
}

long Chart::screenX(double x) {
  return (x-x_min)*xscale+rc.left+BORDER_X;
}

long Chart::screenY(double y) {
  return (y_max-y)*yscale+rc.top;
}

long Chart::screenS(double s) {
  return s*yscale;
}


void Chart::DrawArrow(const double x, const double y, 
		      const double mag, const double angle,
		      const int Style) {
  POINT wv[2];
  double dX, dY;

  wv[0].x = screenX(x);
  wv[0].y = screenY(y);
  
  dX = mag;
  dY = 0;
  rotate(dX,dY,angle);
  wv[1].x = (int)(wv[0].x + dX);
  wv[1].y = (int)(wv[0].y + dY);
  StyleLine(wv[0], wv[1], Style);

  dX = mag-5;
  dY = -3;
  rotate(dX,dY,angle);
  wv[1].x = (int)(wv[0].x + dX);
  wv[1].y = (int)(wv[0].y + dY);
  StyleLine(wv[0], wv[1], Style);

  dX = mag-5;
  dY = 3;
  rotate(dX,dY,angle);
  wv[1].x = (int)(wv[0].x + dX);
  wv[1].y = (int)(wv[0].y + dY);  
  StyleLine(wv[0], wv[1], Style);
}
