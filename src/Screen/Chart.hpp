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

#ifndef CHART_HPP
#define CHART_HPP

#include "Math/leastsqs.h"
#include "Screen/Canvas.hpp"

class Chart {
 private:
  Canvas &canvas;
  RECT rc;
 public:
  Chart(Canvas &the_canvas, const RECT the_rc);

  static const Color GROUND_COLOUR;

  enum {
    STYLE_BLUETHIN,
    STYLE_REDTHICK,
    STYLE_DASHGREEN,
    STYLE_MEDIUMBLACK,
    STYLE_THINDASHPAPER
  };

  void Reset();

  void DrawBarChart(const LeastSquares &lsdata);

  void DrawFilledLineGraph(const LeastSquares &lsdata, const Color thecolor);

  void DrawLineGraph(const LeastSquares &lsdata,
		     const int Style);
  void DrawTrend(const LeastSquares &lsdata,
                        const int Style);
  void DrawTrendN(const LeastSquares &lsdata,
                         const int Style);

  void DrawLine(const double xmin,
		const double ymin,
		const double xmax,
		const double ymax, const int Style);

  void ScaleYFromData(const LeastSquares &lsdata);
  void ScaleXFromData(const LeastSquares &lsdata);
  void ScaleYFromValue(const double val);
  void ScaleXFromValue(const double val);
  void ScaleMakeSquare();

  void StyleLine(const POINT l1, const POINT l2,
		 const int Style);

  void ResetScale();

  void FormatTicText(TCHAR *text, const double val, const double step);
  void DrawXGrid(const double tic_step,
		 const double zero,
		 const int Style,
		 const double unit_step,
		 bool draw_units=false);
  void DrawYGrid(const double tic_step,
		 const double zero,
		 const int Style,
		 const double unit_step, bool draw_units=false);

  void DrawXLabel(const TCHAR *text);
  void DrawYLabel(const TCHAR *text);
  void DrawLabel(const TCHAR *text,
		 const double xv, const double yv);

  void DrawArrow(const double x, const double y,
		 const double mag, const double angle,
		 const int Style);

  void DrawNoData();
  double getYmin() { return y_min; }
  double getYmax() { return y_max; }
  double getXmin() { return x_min; }
  double getXmax() { return x_max; }
  long screenX(double x);
  long screenY(double y);
  long screenS(double s);
private:
  double yscale;
  double xscale;
  double y_min, x_min;
  double x_max, y_max;
  bool unscaled_x;
  bool unscaled_y;
};

#endif
