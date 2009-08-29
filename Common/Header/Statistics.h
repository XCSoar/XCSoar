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

#ifndef STATISTICS_H
#define STATISTICS_H

#include <windows.h>
#include "Math/leastsqs.h"
#include "Task.h"
#include "Screen/Canvas.hpp"

class Statistics {
 public:

  enum {
    STYLE_BLUETHIN,
    STYLE_REDTHICK,
    STYLE_DASHGREEN,
    STYLE_MEDIUMBLACK,
    STYLE_THINDASHPAPER
  };

  LeastSquares ThermalAverage;
  LeastSquares Wind_x;
  LeastSquares Wind_y;
  LeastSquares Altitude;
  LeastSquares Altitude_Base;
  LeastSquares Altitude_Ceiling;
  LeastSquares Task_Speed;
  double LegStartTime[MAXTASKPOINTS];
  LeastSquares Altitude_Terrain;

  void Reset();

  static void DrawBarChart(Canvas &canvas, const RECT rc, LeastSquares* lsdata);

  static void DrawFilledLineGraph(Canvas &canvas, const RECT rc,
				  LeastSquares* lsdata,
				  const COLORREF thecolor);

  static void DrawLineGraph(Canvas &canvas, const RECT rc, LeastSquares* lsdata,
                            const int Style);
  static void DrawTrend(Canvas &canvas, const RECT rc, LeastSquares* lsdata,
                        const int Style);
  static void DrawTrendN(Canvas &canvas, const RECT rc, LeastSquares* lsdata,
                         const int Style);

  static void DrawLine(Canvas &canvas, RECT rc,
		       const double xmin,
		       const double ymin,
                       const double xmax,
		       const double ymax, const int Style);

  static void ScaleYFromData(const RECT rc, LeastSquares* lsdata);
  static void ScaleXFromData(const RECT rc, LeastSquares* lsdata);
  static void ScaleYFromValue(const RECT rc, const double val);
  static void ScaleXFromValue(const RECT rc, const double val);
  static void ScaleMakeSquare(const RECT rc);

  static void StyleLine(Canvas &canvas, const POINT l1, const POINT l2, const int Style, const RECT rc);

  static double yscale;
  static double xscale;
  static double y_min, x_min;
  static double x_max, y_max;
  static bool unscaled_x;
  static bool unscaled_y;
  static void ResetScale();

  static void FormatTicText(TCHAR *text, const double val, const double step);
  static void DrawXGrid(Canvas &canvas, const RECT rc,
			const double tic_step,
			const double zero,
                        const int Style,
			const double unit_step,
			bool draw_units=false);
  static void DrawYGrid(Canvas &canvas, const RECT rc,
			const double tic_step,
			const double zero,
                        const int Style,
			const double unit_step, bool draw_units=false);

  static void DrawXLabel(Canvas &canvas, const RECT rc, const TCHAR *text);
  static void DrawYLabel(Canvas &canvas, const RECT rc, const TCHAR *text);
  static void DrawLabel(Canvas &canvas, const RECT rc, const TCHAR *text,
			const double xv, const double yv);
  static void DrawNoData(Canvas &canvas, const RECT rc);

  ///

    static void RenderAirspace(Canvas &canvas, const RECT rc);
    static void RenderBarograph(Canvas &canvas, const RECT rc);
    static void RenderClimb(Canvas &canvas, const RECT rc);
    static void RenderGlidePolar(Canvas &canvas, const RECT rc);
    static void RenderWind(Canvas &canvas, const RECT rc);
    static void RenderTemperature(Canvas &canvas, const RECT rc);
    static void RenderTask(Canvas &canvas, const RECT rc, const bool olcmode);
    static void RenderSpeed(Canvas &canvas, const RECT rc);

};


LRESULT CALLBACK AnalysisProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


#endif
