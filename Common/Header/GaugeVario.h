#ifndef GAUGE_VARIO_H
#define GAUGE_VARIO_H
#include "stdafx.h"

class GaugeVario {
 public:
  static void Create();
  static void Destroy();
  static void Render();
  static void RenderBg();
 private:
  static HDC hdcScreen;
  static HDC hdcDrawWindow;
  static HBITMAP hDrawBitMap;
  static RECT rc;
};

#endif
