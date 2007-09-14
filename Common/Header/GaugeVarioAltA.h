#ifndef GAUGE_VARIO_H
#define GAUGE_VARIO_H
#include "stdafx.h"

extern HWND hWndVarioWindow;

typedef struct{
  bool    InitDone;
  int     maxLabelSize;
  RECT    recBkg;
  POINT   orgText;
  double  lastValue;
  TCHAR   lastText[32];
  HBITMAP lastBitMap;
}DrawInfo_t;

class GaugeVario {
 public:
  static void Create();
  static void Destroy();
  static void Render();
  static void RenderBg();
  static void Repaint(HDC hDC);
  static void Show(bool doshow);

 private:
  static void RenderZero(void);
  static void RenderValue(int x, int y, DrawInfo_t *diValue, DrawInfo_t *diLabel, double Value, TCHAR *Label);
  static void RenderSpeedToFly(int x, int y);
  static void RenderBallast(void);
  static void RenderBugs(void);
  static void RenderNeedle(double Value, bool average);
  static void RenderClimb(void);

  static int xoffset;
  static int yoffset;
  static int gmax;
  static void MakePolygon(const int i);
  static void MakeAllPolygons();
  static POINT* getPolygon(const int i);
  static POINT *polys;
  static bool dirty;
  static HDC hdcScreen;
  static HDC hdcDrawWindow;
  static HDC hdcTemp;
  static HBITMAP hDrawBitMap;
  static RECT rc;
  static DrawInfo_t diValueTop;
  static DrawInfo_t diValueMiddle;
  static DrawInfo_t diValueBottom;
  static DrawInfo_t diLabelTop;
  static DrawInfo_t diLabelMiddle;
  static DrawInfo_t diLabelBottom;
  static HBITMAP hBitmapUnit;
  static HBITMAP hBitmapClimb;
  static POINT BitmapUnitPos;
  static POINT BitmapUnitSize;
  static HBRUSH redBrush;
  static HBRUSH blueBrush;
  static HPEN redPen;
  static HPEN bluePen;

};

#endif
