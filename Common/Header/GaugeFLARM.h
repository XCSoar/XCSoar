#ifndef GAUGE_FLARM_H
#define GAUGE_FLARM_H
#include "StdAfx.h"
#include "Parser.h"

extern HWND hWndFLARMWindow;


class GaugeFLARM {
 public:
  static void Create();
  static void Destroy();
  static void Render(NMEA_INFO* gps_info);
  static void RenderTraffic(NMEA_INFO *gps_info);
  static void RenderBg();
  static void Repaint(HDC hDC);
  static void Show();
  static bool Enable;
  static bool Suppress;
  static void TrafficPresent(bool traffic);
  static bool ForceVisible;
 private:
  static HDC hdcScreen;
  static HDC hdcDrawWindow;
  static HDC hdcTemp;
  static HBITMAP hDrawBitMap;
  static HBITMAP hRoseBitMap;
  static RECT rc;
  static int radius;
  static POINT center;
  static bool Traffic;
  static int RangeScale(double d);
};

extern DWORD EnableFLARMDisplay;
extern DWORD FLARMGaugeBearing;

#endif
