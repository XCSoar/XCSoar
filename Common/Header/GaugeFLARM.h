#ifndef GAUGE_FLARM_H
#define GAUGE_FLARM_H

#include "Parser.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class GaugeFLARM {
 public:
  static void Create();
  static void Destroy();
  static void Render(NMEA_INFO* gps_info);
  static void RenderTraffic(NMEA_INFO *gps_info);
  static void RenderBg();
  static void Repaint(HDC hDC);
  static void Show();
  static bool Visible;
  static bool Suppress;
  static void TrafficPresent(bool traffic);
  static bool ForceVisible;
 private:
  static HWND hWndFLARMWindow;
  static HDC hdcScreen;
  static HDC hdcDrawWindow;
  static HDC hdcTemp;
  static HBITMAP hDrawBitMap;
  static HBITMAP hRoseBitMap;
  static int hRoseBitMapWidth;
  static int hRoseBitMapHeight;
  static RECT rc;
  static int radius;
  static POINT center;
  static bool Traffic;
  static int RangeScale(double d);
};

extern bool EnableFLARMGauge;
extern DWORD EnableFLARMMap;

#endif
