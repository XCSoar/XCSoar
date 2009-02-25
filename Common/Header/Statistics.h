#ifndef STATISTICS_H
#define STATISTICS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "leastsqs.h"
#include "Task.h"

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

  static void DrawBarChart(HDC hdc, const RECT rc, LeastSquares* lsdata);

  static void DrawFilledLineGraph(HDC hdc, const RECT rc,
				  LeastSquares* lsdata,
				  const COLORREF thecolor);

  static void DrawLineGraph(HDC hdc, const RECT rc, LeastSquares* lsdata,
                            const int Style);
  static void DrawTrend(HDC hdc, const RECT rc, LeastSquares* lsdata,
                        const int Style);
  static void DrawTrendN(HDC hdc, const RECT rc, LeastSquares* lsdata,
                         const int Style);

  static void DrawLine(HDC hdc, RECT rc,
		       const double xmin,
		       const double ymin,
                       const double xmax,
		       const double ymax, const int Style);

  static void ScaleYFromData(const RECT rc, LeastSquares* lsdata);
  static void ScaleXFromData(const RECT rc, LeastSquares* lsdata);
  static void ScaleYFromValue(const RECT rc, const double val);
  static void ScaleXFromValue(const RECT rc, const double val);
  static void ScaleMakeSquare(const RECT rc);

  static void StyleLine(HDC hdc, const POINT l1, const POINT l2, const int Style, const RECT rc);

  static double yscale;
  static double xscale;
  static double y_min, x_min;
  static double x_max, y_max;
  static bool unscaled_x;
  static bool unscaled_y;
  static void ResetScale();

  static void FormatTicText(TCHAR *text, const double val, const double step);
  static void DrawXGrid(HDC hdc, const RECT rc,
			const double tic_step,
			const double zero,
                        const int Style,
			const double unit_step,
			bool draw_units=false);
  static void DrawYGrid(HDC hdc, const RECT rc,
			const double tic_step,
			const double zero,
                        const int Style,
			const double unit_step, bool draw_units=false);

  static void DrawXLabel(HDC hdc, const RECT rc, const TCHAR *text);
  static void DrawYLabel(HDC hdc, const RECT rc, const TCHAR *text);
  static void DrawLabel(HDC hdc, const RECT rc, const TCHAR *text,
			const double xv, const double yv);
  static void DrawNoData(HDC hdc, const RECT rc);

  ///

    static void RenderAirspace(HDC hdc, const RECT rc);
    static void RenderBarograph(HDC hdc, const RECT rc);
    static void RenderClimb(HDC hdc, const RECT rc);
    static void RenderGlidePolar(HDC hdc, const RECT rc);
    static void RenderWind(HDC hdc, const RECT rc);
    static void RenderTemperature(HDC hdc, const RECT rc);
    static void RenderTask(HDC hdc, const RECT rc, const bool olcmode);
    static void RenderSpeed(HDC hdc, const RECT rc);

};


LRESULT CALLBACK AnalysisProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


#endif
