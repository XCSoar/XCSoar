#ifndef GAUGE_FLARM_H
#define GAUGE_FLARM_H
#include "stdafx.h"
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
  static void Show(bool doshow);
  static bool Enable;
 private:
  static HDC hdcScreen;
  static HDC hdcDrawWindow;
  static HDC hdcTemp;
  static HBITMAP hDrawBitMap;
  static HBITMAP hRoseBitMap;
  static RECT rc;
  static int radius; 
  static POINT center;
  static int RangeScale(double d);
};

#endif
