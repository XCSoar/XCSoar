#include "GaugeVario.h"
#include "MapWindow.h"
#include "Utils.h"
#include "externs.h"

HWND hWndVarioWindow = NULL; // Vario Window
extern HINSTANCE hInst;      // The current instance
extern HWND hWndMainWindow; // Main Windows
extern HFONT CDIWindowFont; // New
extern HWND hWndMenuButton;

extern HFONT InfoWindowFont;

HDC GaugeVario::hdcScreen = NULL;
HDC GaugeVario::hdcDrawWindow = NULL;
HBITMAP GaugeVario::hDrawBitMap = NULL;
RECT GaugeVario::rc;

#define GAUGEXSIZE 65
#define GAUGEYSIZE 100

void GaugeVario::Create() {
  RECT bigrc;

  bigrc = MapWindow::MapRect;

  hWndVarioWindow = CreateWindow(TEXT("STATIC"),TEXT(" "),
			       WS_VISIBLE|WS_CHILD | WS_CLIPCHILDREN 
			       | WS_CLIPSIBLINGS,
                               0,0,0,0,
			       hWndMainWindow,NULL,hInst,NULL);
  SendMessage(hWndVarioWindow,WM_SETFONT,
              (WPARAM)CDIWindowFont,MAKELPARAM(TRUE,0));
  SetWindowPos(hWndVarioWindow,hWndMenuButton,
               bigrc.right-GAUGEXSIZE,bigrc.top+50,
               GAUGEXSIZE,GAUGEYSIZE,
	       SWP_SHOWWINDOW);

  GetClientRect(hWndVarioWindow, &rc);

  // 
  hdcScreen = GetDC(hWndVarioWindow);
  hdcDrawWindow = CreateCompatibleDC(hdcScreen);
  hDrawBitMap = CreateCompatibleBitmap (hdcScreen, GAUGEXSIZE, GAUGEYSIZE);

}


void GaugeVario::Destroy() {
  ReleaseDC(hWndVarioWindow, hdcScreen);
  DeleteDC(hdcDrawWindow);
  DeleteObject(hDrawBitMap);
  DestroyWindow(hWndVarioWindow);
}

#define GAUGEVARIORANGE 2.50 // 5 m/s
#define GAUGEVARIOSWEEP 80 // degrees total sweep

extern NMEA_INFO     GPS_INFO;
extern DERIVED_INFO  CALCULATED_INFO;

//extern DERIVED_INFO DerivedDrawInfo;
//extern NMEA_INFO DrawInfo;

void GaugeVario::Render() {

  SelectObject(hdcDrawWindow, (HBITMAP)hDrawBitMap);
  RenderBg();

  SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
  SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));

  // draw dashes
  POINT bit[3];
  int i;
  int xoffset = 80;
  int yoffset = (rc.bottom-rc.top)/2;
  int degrees_per_unit = (int)((GAUGEVARIOSWEEP/2.0)/(GAUGEVARIORANGE*LIFTMODIFY));
  int gmax = (int)(degrees_per_unit*(GAUGEVARIORANGE*LIFTMODIFY));
  double dx, dy;
  double vval;

  if (GPS_INFO.VarioAvailable) {
    vval = GPS_INFO.Vario;
  } else {
    vval = CALCULATED_INFO.Vario;
  }
  i = (int)(vval*degrees_per_unit*LIFTMODIFY);
  i = min(gmax,max(-gmax,i));

  dx = -xoffset+15; dy = 6;
  rotate(&dx, &dy, i);
  bit[0].x = (int)(dx+xoffset); bit[0].y = (int)(dy+yoffset);

  dx = -xoffset+15; dy = -6;
  rotate(&dx, &dy, i);
  bit[1].x = (int)(dx+xoffset); bit[1].y = (int)(dy+yoffset);

  dx = -xoffset+2; dy = 0;
  rotate(&dx, &dy, i);
  bit[2].x = (int)(dx+xoffset); bit[2].y = (int)(dy+yoffset);

  Polygon(hdcDrawWindow, bit, 3);

  TCHAR Temp[10];
  SIZE tsize;

  wsprintf(Temp,TEXT("%2.1f"), vval*LIFTMODIFY);
  SelectObject(hdcDrawWindow, InfoWindowFont);
  GetTextExtentPoint(hdcDrawWindow, Temp, _tcslen(Temp), &tsize);

  ExtTextOut(hdcDrawWindow, rc.right-tsize.cx-2, yoffset-tsize.cy/2, 
	     0, NULL, Temp, _tcslen(Temp), NULL);

  BitBlt(hdcScreen, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);

}


void GaugeVario::RenderBg() {

  SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
  SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
  Rectangle(hdcDrawWindow,0,0,GAUGEXSIZE,GAUGEYSIZE);

  SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));

  // draw dashes
  POINT bit[2];
  int i;
  int xoffset = 80;
  int yoffset = (rc.bottom-rc.top)/2;
  int degrees_per_unit = (int)((GAUGEVARIOSWEEP/2.0)/(GAUGEVARIORANGE*LIFTMODIFY));
  int gmax = (int)(degrees_per_unit*(GAUGEVARIORANGE*LIFTMODIFY));
  double dx, dy;
  for (i= 0; i<= gmax; i+= degrees_per_unit) {
    if (i==0) {
      dx = -xoffset+15; dy = 0; 
    } else {
      dx = -xoffset+5; dy = 0; 
    }
    rotate(&dx, &dy, i);
    bit[0].x = (int)(dx+xoffset); bit[0].y = (int)(dy+yoffset);

    dx = -xoffset; dy = 0; rotate(&dx, &dy, i);
    bit[1].x = (int)(dx+xoffset); bit[1].y = (int)(dy+yoffset);

    Polyline(hdcDrawWindow, bit, 2);
  }
  for (i= 0; i>= -gmax; i-= degrees_per_unit) {
    if (i==0) {
      dx = -xoffset+15; dy = 0; 
    } else {
      dx = -xoffset+5; dy = 0; 
    }
    rotate(&dx, &dy, i);
    bit[0].x = (int)(dx+xoffset); bit[0].y = (int)(dy+yoffset);

    dx = -xoffset; dy = 0; rotate(&dx, &dy, i);
    bit[1].x = (int)(dx+xoffset); bit[1].y = (int)(dy+yoffset);

    Polyline(hdcDrawWindow, bit, 2);
  }

}
