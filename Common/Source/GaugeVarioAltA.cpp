/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"
#include "GaugeVarioAltA.h"
#include "MapWindow.h"
#include "Utils.h"
#include "externs.h"
#include "InfoBoxLayout.h"

extern NMEA_INFO DrawInfo;
extern DERIVED_INFO DerivedDrawInfo;


HWND   hWndVarioWindow = NULL; // Vario Window
extern HINSTANCE hInst;      // The current instance
extern HWND hWndMainWindow; // Main Windows
extern HFONT CDIWindowFont; // New
extern HWND hWndMenuButton;

extern HFONT InfoWindowFont;
extern HFONT TitleWindowFont;
extern HFONT TitleWindowFont;
extern HFONT MapWindowFont;
extern HFONT MapWindowBoldFont;

HDC GaugeVario::hdcScreen = NULL;
HDC GaugeVario::hdcDrawWindow = NULL;
HDC GaugeVario::hdcTemp = NULL;
HBITMAP GaugeVario::hDrawBitMap = NULL;
RECT GaugeVario::rc;

HBITMAP GaugeVario::hBitmapUnit;
POINT GaugeVario::BitmapUnitPos;
POINT GaugeVario::BitmapUnitSize;


DrawInfo_t GaugeVario::diValueTop = {false};
DrawInfo_t GaugeVario::diValueMiddle = {false};
DrawInfo_t GaugeVario::diValueBottom = {false};
DrawInfo_t GaugeVario::diLabelTop = {false};
DrawInfo_t GaugeVario::diLabelMiddle = {false};
DrawInfo_t GaugeVario::diLabelBottom = {false};

#define GAUGEXSIZE (InfoBoxLayout::ControlWidth)
#define GAUGEYSIZE (InfoBoxLayout::ControlHeight*3)

COLORREF colTextGray;
COLORREF colText;
COLORREF colTextBackgnd;

LRESULT CALLBACK GaugeVarioWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


void GaugeVario::Create() {
  RECT bigrc;

  bigrc = MapWindow::MapRect;

  hWndVarioWindow = CreateWindow(TEXT("STATIC"),TEXT(" "),
			       WS_VISIBLE|WS_CHILD | WS_CLIPCHILDREN
			       | WS_CLIPSIBLINGS,
                               0,0,0,0,
			       hWndMainWindow,NULL,hInst,NULL);
  SetWindowPos(hWndVarioWindow,hWndMenuButton,
               bigrc.right+InfoBoxLayout::ControlWidth,
	       bigrc.top,
               GAUGEXSIZE,GAUGEYSIZE,
	       SWP_SHOWWINDOW);

  GetClientRect(hWndVarioWindow, &rc);

  hdcScreen = GetDC(hWndVarioWindow);                       // the screen DC
  hdcDrawWindow = CreateCompatibleDC(hdcScreen);            // the memory DC
  hdcTemp = CreateCompatibleDC(hdcScreen);                  // temp DC to select Uniz Bmp's

                                                            // prepare drawing DC, setup size and coler deep
  HBITMAP memBM = CreateCompatibleBitmap (hdcScreen, rc.right-rc.left, rc.bottom-rc.top);
  SelectObject(hdcDrawWindow, memBM);
                                                            // load vario scale
  hDrawBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_VARIOSCALEA));

  if (Appearance.InverseInfoBox){
    colText = RGB(0xff, 0xff, 0xff);
    colTextBackgnd = RGB(0x00, 0x00, 0x00);
    colTextGray = RGB(0xa0, 0xa0, 0xa0);
  } else {
    colText = RGB(0x00, 0x00, 0x00);
    colTextBackgnd = RGB(0xff, 0xff, 0xff);
    colTextGray = RGB(~0xa0, ~0xa0, ~0xa0);
  }

  SetTextColor(hdcDrawWindow, colText);
  SetBkColor(hdcDrawWindow, colTextBackgnd);

  if (Appearance.InverseInfoBox){
    Units::GetUnitBitmap(Units::GetUserUnitByGroup(ugVerticalSpeed),
      &hBitmapUnit, &BitmapUnitPos, &BitmapUnitSize, UNITBITMAPINVERS | UNITBITMAPGRAY);
  } else {
    Units::GetUnitBitmap(Units::GetUserUnitByGroup(ugVerticalSpeed),
      &hBitmapUnit, &BitmapUnitPos, &BitmapUnitSize, UNITBITMAPGRAY);
  }

  SetWindowLong(hWndVarioWindow, GWL_WNDPROC, (LONG) GaugeVarioWndProc);

  Render();

}


void GaugeVario::Destroy() {
  ReleaseDC(hWndVarioWindow, hdcScreen);
  DeleteDC(hdcDrawWindow);
  DeleteDC(hdcTemp);
  DeleteObject(hDrawBitMap);
  DestroyWindow(hWndVarioWindow);
}

#define GAUGEVARIORANGE 5.0 //2.50 // 5 m/s
#define GAUGEVARIOSWEEP 180 // degrees total sweep

extern NMEA_INFO     GPS_INFO;
extern DERIVED_INFO  CALCULATED_INFO;


void GaugeVario::Render() {

  static POINT orgTop     = {-1,-1};
  static POINT orgMiddle  = {-1,-1};
  static POINT orgBottom  = {-1,-1};
  static int xoffset;
  static int yoffset;
  static int ValueHeight;
  static InitDone = false;
  
  double vval;

//  HKEY Key;

//  RenderBg();                                          // ???

  if (!InitDone){
    HBITMAP oldBmp;
    
    xoffset = (rc.right-rc.left);
    yoffset = (rc.bottom-rc.top)/2;
    ValueHeight = (1 + Appearance.CDIWindowFont.CapitalHeight + 2 + Appearance.TitleWindowFont.CapitalHeight + 1);
    orgTop.y = (ValueHeight/2 + ValueHeight);
    orgTop.x = rc.right;
    orgMiddle.y = orgTop.y + ValueHeight;
    orgMiddle.x = rc.right;
    orgBottom.y = orgMiddle.y + ValueHeight;
    orgBottom.x = rc.right;

    oldBmp = (HBITMAP)SelectObject(hdcTemp, (HBITMAP)hDrawBitMap); 
    // copy scale bitmap to memory DC
    if (InfoBoxLayout::scale>1) {
      if (Appearance.InverseInfoBox)
	StretchBlt(hdcDrawWindow, 0, 0, rc.right, rc.bottom, 
		   hdcTemp,
		   58, 0,
		   58, 120, 
		   SRCCOPY);
      else
	StretchBlt(hdcDrawWindow, 0, 0, rc.right, rc.bottom, 
		   hdcTemp, 
		   0, 0, 
		   58, 120,
		   SRCCOPY);
    } else {

      if (Appearance.InverseInfoBox)
	BitBlt(hdcDrawWindow, 0, 0, rc.right, rc.bottom, 
	       hdcTemp, 58, 0, SRCCOPY);
      else
	BitBlt(hdcDrawWindow, 0, 0, rc.right, rc.bottom, 
	       hdcTemp, 0, 0, SRCCOPY);

    }
      
    SelectObject(hdcTemp, oldBmp);

    InitDone = true;
  }

  if (GPS_INFO.VarioAvailable) {
    vval = GPS_INFO.Vario;
  } else {
    vval = CALCULATED_INFO.Vario;
  }

  RenderNeedle(vval, xoffset, yoffset);

  vval = vval*LIFTMODIFY;
  vval = min(99.9,max(-99.9,vval));

  if (Appearance.GaugeVarioAvgText) {
    RenderValue(orgTop.x, orgTop.y, &diValueTop, &diLabelTop, 
		CALCULATED_INFO.Average30s*LIFTMODIFY, TEXT("Avg"));
  }

  RenderValue(orgMiddle.x, orgMiddle.y, &diValueMiddle, &diLabelMiddle, vval, 
	      TEXT("Gross"));

  if (Appearance.GaugeVarioMc) {
    if (CALCULATED_INFO.AutoMacCready)
      RenderValue(orgBottom.x, orgBottom.y, 
		  &diValueBottom, &diLabelBottom, MACCREADY*LIFTMODIFY, TEXT("Auto Mc"));
    else
      RenderValue(orgBottom.x, orgBottom.y, 
		  &diValueBottom, &diLabelBottom, MACCREADY*LIFTMODIFY, TEXT("Mc"));
  }

  if (Appearance.GaugeVarioSpeedToFly) {
    RenderSpeedToFly(rc.right - 11, (rc.bottom-rc.top)/2);
  }

  if (Appearance.GaugeVarioBallast) {
    RenderBallast();
  }

  if (Appearance.GaugeVarioBugs) {
    RenderBugs();
  }

  BitBlt(hdcScreen, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);

}

void GaugeVario::Repaint(HDC hDC){
  BitBlt(hDC, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);
}

void GaugeVario::RenderBg() {
}

void GaugeVario::RenderNeedle(double Value, int x, int y){

  static int lastI = -99999;
  static POINT lastBit[3];
  static bool InitDone = false;
  static int degrees_per_unit;
  static int gmax;
  static first = true;
  static int nlength0, nlength1, nwidth;

  if (first) {
    first = false;
    if (Appearance.GaugeVarioNeedleStyle == gvnsLongNeedle) {
      nlength0 = 25*InfoBoxLayout::scale;
      nlength1 = 6*InfoBoxLayout::scale;
      nwidth = 3*InfoBoxLayout::scale;
    } else {
      nlength0 = 13*InfoBoxLayout::scale;
      nlength1 = 6*InfoBoxLayout::scale;
      nwidth = 4*InfoBoxLayout::scale;
    }
  }
  
  POINT bit[3];
  int i;
  double dx, dy;

  if (!InitDone){
    degrees_per_unit = (int)((GAUGEVARIOSWEEP/2.0)/(GAUGEVARIORANGE*LIFTMODIFY));
    gmax = (int)(degrees_per_unit*(GAUGEVARIORANGE*LIFTMODIFY))+2;
    InitDone = true;
  }

  i = (int)(Value*degrees_per_unit*LIFTMODIFY);
  i = min(gmax,max(-gmax,i));

  if (i != lastI){

    if (lastI != -99999){
      if (Appearance.InverseInfoBox){
        SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
        SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
      } else {
        SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
        SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
      }

      Polygon(hdcDrawWindow, lastBit, 3);
    }

    if (Appearance.InverseInfoBox){
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    }

    dx = -x+nlength0; dy = nwidth;
    rotate(&dx, &dy, i);
    bit[0].x = (int)(dx+x); bit[0].y = (int)(dy+y);

    dx = -x+nlength0; dy = -nwidth;
    rotate(&dx, &dy, i);
    bit[1].x = (int)(dx+x); bit[1].y = (int)(dy+y);

    dx = -x+nlength1; dy = 0;
    rotate(&dx, &dy, i);
    bit[2].x = (int)(dx+x); bit[2].y = (int)(dy+y);

    Polygon(hdcDrawWindow, bit, 3);

    memcpy(lastBit, bit, sizeof(bit));

    lastI = i;
  }

}


void GaugeVario::RenderValue(int x, int y, DrawInfo_t *diValue, DrawInfo_t *diLabel, double Value, TCHAR *Label){

  SIZE tsize;


  if (!diValue->InitDone){

    diValue->recBkg.right = x-5*InfoBoxLayout::scale;
    diValue->recBkg.top = y +3*InfoBoxLayout::scale
      + Appearance.TitleWindowFont.CapitalHeight;

    diValue->recBkg.left = diValue->recBkg.right;           // update back rect with max label size
    diValue->recBkg.bottom = diValue->recBkg.top 
      + Appearance.CDIWindowFont.CapitalHeight;

    diValue->orgText.x = diValue->recBkg.left;
    diValue->orgText.y = diValue->recBkg.top 
      + Appearance.CDIWindowFont.CapitalHeight 
      - Appearance.CDIWindowFont.AscentHeight;

    diValue->lastValue = -9999;
    diValue->lastText[0] = '\0';
    diValue->lastBitMap = NULL;
    diValue->InitDone = true;
  }

  if (!diLabel->InitDone){

    diLabel->recBkg.right = x;
    diLabel->recBkg.top = y+1*InfoBoxLayout::scale;

    diLabel->recBkg.left = diLabel->recBkg.right;           // update back rect with max label size
    diLabel->recBkg.bottom = diLabel->recBkg.top 
      + Appearance.TitleWindowFont.CapitalHeight;

    diLabel->orgText.x = diLabel->recBkg.left;
    diLabel->orgText.y = diLabel->recBkg.top 
      + Appearance.TitleWindowFont.CapitalHeight 
      - Appearance.TitleWindowFont.AscentHeight;

    diLabel->lastValue = -9999;
    diLabel->lastText[0] = '\0';
    diLabel->lastBitMap = NULL;
    diLabel->InitDone = true;
  }

  SetBkMode(hdcDrawWindow, TRANSPARENT);

  if (_tcscmp(diLabel->lastText, Label) != 0){

    SetBkColor(hdcDrawWindow, colTextBackgnd);

    SetTextColor(hdcDrawWindow, colTextGray);

    SelectObject(hdcDrawWindow, TitleWindowFont);

    GetTextExtentPoint(hdcDrawWindow, Label, _tcslen(Label), &tsize);
    diLabel->orgText.x = diLabel->recBkg.right - tsize.cx;

    ExtTextOut(hdcDrawWindow, diLabel->orgText.x, diLabel->orgText.y,
      ETO_OPAQUE, &diLabel->recBkg, Label, _tcslen(Label), NULL);

    diLabel->recBkg.left = diLabel->orgText.x;

    _tcscpy(diLabel->lastText, Label);

  }

  if (diValue->lastValue != Value){

    TCHAR Temp[18];

    SetBkColor(hdcDrawWindow, colTextBackgnd);

    SetTextColor(hdcDrawWindow, colText);

    _stprintf(Temp, TEXT("%.1f"), Value);

    SelectObject(hdcDrawWindow, CDIWindowFont);

    GetTextExtentPoint(hdcDrawWindow, Temp, _tcslen(Temp), &tsize);
    diValue->orgText.x = diValue->recBkg.right - tsize.cx;

    ExtTextOut(hdcDrawWindow, diValue->orgText.x, diValue->orgText.y,
      ETO_OPAQUE, &diValue->recBkg, Temp, _tcslen(Temp), NULL);

    diValue->recBkg.left = diValue->orgText.x;

    diValue->lastValue = Value;
  }

  if (diLabel->lastBitMap != hBitmapUnit){
    HBITMAP oldBmp;
    
    oldBmp = (HBITMAP)SelectObject(hdcTemp, hBitmapUnit);
    BitBlt(hdcDrawWindow,
	   x-5*InfoBoxLayout::scale, 
	   diValue->recBkg.top,
      BitmapUnitSize.x, BitmapUnitSize.y,
      hdcTemp,
      BitmapUnitPos.x, BitmapUnitPos.y,
      SRCCOPY
    );
    SelectObject(hdcTemp, oldBmp);
    diLabel->lastBitMap = hBitmapUnit;
  }

}

void GaugeVario::RenderSpeedToFly(int x, int y){

  #define  YOFFSET     36
  #define  DeltaVstep  4
  #define  DeltaVlimit 16

#ifndef _SIM_
  if (!(DrawInfo.AirspeedAvailable && DrawInfo.VarioAvailable)) {
    return;
  }
#else
  // cheat
  DrawInfo.IndicatedAirspeed = DrawInfo.Speed;
#endif

  static double lastVdiff;
  double vdiff;

  if (DerivedDrawInfo.Flying && !DerivedDrawInfo.Circling){
    vdiff = (DerivedDrawInfo.VOpt - DrawInfo.IndicatedAirspeed);
    vdiff = max(-DeltaVlimit, min(DeltaVlimit, vdiff)); // limit it
    vdiff = iround(vdiff/DeltaVstep) * DeltaVstep;
  } else
    vdiff = 0;

  if (lastVdiff != vdiff){

    if (Appearance.InverseInfoBox){
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    }
    SetBkMode(hdcDrawWindow, OPAQUE);

    // ToDo sgi optimize
    Rectangle(hdcDrawWindow, x, y+YOFFSET, x+11, y+YOFFSET+4*4);
    Rectangle(hdcDrawWindow, x, y-YOFFSET-(4*4-1), x+11, y-YOFFSET+1);

    if (Appearance.InverseInfoBox){
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    }

    if (vdiff > 0){ // to slow

      y += YOFFSET;

      while (vdiff > 0){
        if (vdiff > DeltaVstep){
          Rectangle(hdcDrawWindow, x, y, x+11, y+3);
        } else {
          POINT Arrow[4];
          Arrow[0].x = x; Arrow[0].y = y;
          Arrow[1].x = x+5; Arrow[1].y = y+3;
          Arrow[2].x = x+10; Arrow[2].y = y;
          Arrow[3].x = x; Arrow[3].y = y;
          Polygon(hdcDrawWindow, Arrow, 4);
        }
        vdiff -=DeltaVstep;
        y += 4;
      }

    } else
    if (vdiff < 0){

      y -= YOFFSET;

      while (vdiff < 0){
        if (vdiff < -DeltaVstep){
          Rectangle(hdcDrawWindow, x, y-2, x+11, y+1);
        } else {
          POINT Arrow[4];
          Arrow[0].x = x; Arrow[0].y = y;
          Arrow[1].x = x+5; Arrow[1].y = y-3;
          Arrow[2].x = x+10; Arrow[2].y = y;
          Arrow[3].x = x; Arrow[3].y = y;
          Polygon(hdcDrawWindow, Arrow, 4);
        }
        vdiff +=DeltaVstep;
        y -= 4;
      }

    }

    lastVdiff = vdiff;

  }

}


void GaugeVario::RenderBallast(void){

  #define TextBal TEXT("Bal")

  static double lastBallast = 1;
  static RECT  recLabelBk = {-1,-1,-1,-1};
  static RECT  recValueBk = {-1,-1,-1,-1};
  static POINT orgLabel  = {-1,-1};
  static POINT orgValue  = {-1,-1};

  if (recLabelBk.left == -1){                               // ontime init, origin and background rect

    SIZE tSize;

    orgLabel.x = 1;                                         // position of ballast label
    orgLabel.y = rc.top + 2 + (Appearance.TitleWindowFont.CapitalHeight*2) - Appearance.TitleWindowFont.AscentHeight;

    orgValue.x = 1;                                         // position of ballast value
    orgValue.y = rc.top + 1 + Appearance.TitleWindowFont.CapitalHeight - Appearance.TitleWindowFont.AscentHeight;

    recLabelBk.left = orgLabel.x;                           // set upper left corner
    recLabelBk.top = orgLabel.y + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;
    recValueBk.left = orgValue.x;                           // set upper left corner
    recValueBk.top = orgValue.y + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;

    SelectObject(hdcDrawWindow, TitleWindowFont);           // get max label size
    GetTextExtentPoint(hdcDrawWindow, TextBal, _tcslen(TextBal), &tSize);

    recLabelBk.right = recLabelBk.left + tSize.cx;          // update back rect with max label size
    recLabelBk.bottom = recLabelBk.top + Appearance.TitleWindowFont.CapitalHeight;

                                                            // get max value size
    GetTextExtentPoint(hdcDrawWindow, TEXT("100%"), _tcslen(TEXT("100%")), &tSize);

    recValueBk.right = recValueBk.left + tSize.cx;          // update back rect with max label size
    recValueBk.bottom = recValueBk.top + Appearance.TitleWindowFont.CapitalHeight;

  }


  if (BALLAST != lastBallast){                              // ballast hase been changed

    TCHAR Temp[18];

    SelectObject(hdcDrawWindow, TitleWindowFont);
    SetBkColor(hdcDrawWindow, colTextBackgnd);

    if (lastBallast < 0.001 || BALLAST < 0.001){
      if (BALLAST < 0.001)                                  // new ballast is 0, hide label
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TEXT(""), 0, NULL);
      else {
        SetTextColor(hdcDrawWindow, colTextGray);           // ols ballast was 0, show label
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TextBal, _tcslen(TextBal), NULL);
      }
    }

    if (BALLAST < 0.001)                                    // new ballast 0, hide value
      _stprintf(Temp, TEXT(""));
    else
      _stprintf(Temp, TEXT("%.0f%%"), BALLAST*100);

    SetTextColor(hdcDrawWindow, colText);              // display value
    ExtTextOut(hdcDrawWindow,
      orgValue.x,
      orgValue.y,
      ETO_OPAQUE, &recValueBk, Temp, _tcslen(Temp), NULL);

    lastBallast = BALLAST;

  }

}

void GaugeVario::RenderBugs(void){

  #define TextBug TEXT("Bug")
  static double lastBugs = 1;
  static RECT  recLabelBk = {-1,-1,-1,-1};
  static RECT  recValueBk = {-1,-1,-1,-1};
  static POINT orgLabel  = {-1,-1};
  static POINT orgValue  = {-1,-1};

  if (recLabelBk.left == -1){

    SIZE tSize;

    orgLabel.x = 1;
    orgLabel.y = rc.bottom - 2 - Appearance.TitleWindowFont.CapitalHeight - Appearance.TitleWindowFont.AscentHeight;

    orgValue.x = 1;
    orgValue.y = rc.bottom - 1 - Appearance.TitleWindowFont.AscentHeight;

    recLabelBk.left = orgLabel.x;
    recLabelBk.top = orgLabel.y + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;
    recValueBk.left = orgValue.x;
    recValueBk.top = orgValue.y + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;

    SelectObject(hdcDrawWindow, TitleWindowFont);
    GetTextExtentPoint(hdcDrawWindow, TextBug, _tcslen(TextBug), &tSize);

    recLabelBk.right = recLabelBk.left + tSize.cx;
    recLabelBk.bottom = recLabelBk.top + Appearance.TitleWindowFont.CapitalHeight + Appearance.TitleWindowFont.Height - Appearance.TitleWindowFont.AscentHeight;

    GetTextExtentPoint(hdcDrawWindow, TEXT("100%"), _tcslen(TEXT("100%")), &tSize);

    recValueBk.right = recValueBk.left + tSize.cx;
    recValueBk.bottom = recValueBk.top + Appearance.TitleWindowFont.CapitalHeight;

  }

  if (BUGS != lastBugs){

    TCHAR Temp[18];

    SelectObject(hdcDrawWindow, TitleWindowFont);
    SetBkColor(hdcDrawWindow, colTextBackgnd);

    if (lastBugs > 0.999 || BUGS > 0.999){
      if (BUGS > 0.999)
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TEXT(""), 0, NULL);
      else {
        SetTextColor(hdcDrawWindow, colTextGray);
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TextBug, _tcslen(TextBug), NULL);
      }
    }

    if (BUGS > 0.999)
      _stprintf(Temp, TEXT(""));
    else
      _stprintf(Temp, TEXT("%.0f%%"), (1-BUGS)*100);

    SetTextColor(hdcDrawWindow, colText);

    ExtTextOut(hdcDrawWindow,
    orgValue.x,
    orgValue.y,
    ETO_OPAQUE, &recValueBk, Temp, _tcslen(Temp), NULL);

    lastBugs = BUGS;

  }

}

LRESULT CALLBACK GaugeVarioWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

  PAINTSTRUCT ps;            // structure for paint info
  HDC hDC;                   // handle to graphics device context,
  
  switch (uMsg){

    case WM_PAINT:
      hDC = BeginPaint(hwnd, &ps);
      GaugeVario::Repaint(hDC);
      DeleteDC(hDC);
      EndPaint(hwnd, &ps);
    break;

  }

  return(DefWindowProc (hwnd, uMsg, wParam, lParam));
}



