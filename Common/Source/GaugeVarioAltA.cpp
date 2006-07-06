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
bool GaugeVario::dirty;
int GaugeVario::xoffset;
int GaugeVario::yoffset;
int GaugeVario::gmax;
POINT* GaugeVario::polys=NULL;

HBITMAP GaugeVario::hBitmapUnit;
HBITMAP GaugeVario::hBitmapClimb;
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


#define NARROWS 3
#define ARROWYSIZE IBLSCALE(3)
#define ARROWXSIZE IBLSCALE(5)


LRESULT CALLBACK GaugeVarioWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


void GaugeVario::Create() {
  RECT bigrc;
  bigrc = MapWindow::MapRect;

  hWndVarioWindow = CreateWindow(TEXT("STATIC"),TEXT(" "),
			       WS_VISIBLE|WS_CHILD | WS_CLIPCHILDREN
			       | WS_CLIPSIBLINGS,
                               0,0,0,0,
			       hWndMainWindow,NULL,hInst,NULL);
  SetWindowPos(hWndVarioWindow, HWND_TOP,
               bigrc.right+InfoBoxLayout::ControlWidth,
	       bigrc.top,
               GAUGEXSIZE,GAUGEYSIZE,
	       SWP_HIDEWINDOW);

  GetClientRect(hWndVarioWindow, &rc);

  hdcScreen = GetDC(hWndVarioWindow);       // the screen DC
  hdcDrawWindow = CreateCompatibleDC(hdcScreen);            // the memory DC
  hdcTemp = CreateCompatibleDC(hdcScreen);  // temp DC to select Uniz Bmp's

                                            // prepare drawing DC, setup size and coler deep
  HBITMAP memBM = CreateCompatibleBitmap (hdcScreen, 
					  rc.right-rc.left, 
					  rc.bottom-rc.top);
  SelectObject(hdcDrawWindow, memBM);

  // load vario scale
  if (Units::GetUserVerticalSpeedUnit()==unKnots) {
    hDrawBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_VARIOSCALEC));
  } else {
    hDrawBitMap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_VARIOSCALEA));
  }

  if (Appearance.InverseInfoBox){
    colText = RGB(0xff, 0xff, 0xff);
    colTextBackgnd = RGB(0x00, 0x00, 0x00);
    colTextGray = RGB(0xa0, 0xa0, 0xa0);
    hBitmapClimb = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMBSMALLINV));
  } else {
    colText = RGB(0x00, 0x00, 0x00);
    colTextBackgnd = RGB(0xff, 0xff, 0xff);
    colTextGray = RGB(~0xa0, ~0xa0, ~0xa0);
    hBitmapClimb = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLIMBSMALL));
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

  xoffset = (rc.right-rc.left);
  yoffset = (rc.bottom-rc.top)/2;

  SetWindowLong(hWndVarioWindow, GWL_WNDPROC, (LONG) GaugeVarioWndProc);

  Render();

}

void GaugeVario::Show(bool doshow) {
  EnableVarioGauge = doshow;
  static bool lastvisible = true;
  if (EnableVarioGauge && !lastvisible) {
    ShowWindow(hWndVarioWindow, SW_SHOW);
  }
  if (!EnableVarioGauge && lastvisible) {
    ShowWindow(hWndVarioWindow, SW_HIDE);
  }
  lastvisible = EnableVarioGauge;
}


void GaugeVario::Destroy() {
  if (polys) {
    free(polys);
    polys=NULL;
  }
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
  static int ValueHeight;
  static bool InitDone = false;
  
  double vval;

//  HKEY Key;

//  RenderBg();                                          // ???

  if (!InitDone){
    HBITMAP oldBmp;
    ValueHeight = (4 
		   + Appearance.CDIWindowFont.CapitalHeight 
		   + Appearance.TitleWindowFont.CapitalHeight);

    orgMiddle.y = yoffset - ValueHeight/2;
    orgMiddle.x = rc.right;
    orgTop.y = orgMiddle.y-ValueHeight;
    orgTop.x = rc.right;
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

  double vvaldisplay = min(99.9,max(-99.9,vval*LIFTMODIFY));

  if (Appearance.GaugeVarioAvgText) {
    // JMW averager now displays netto average if not circling
    if (!DerivedDrawInfo.Circling) {
      RenderValue(orgTop.x, orgTop.y, &diValueTop, &diLabelTop, 
		  CALCULATED_INFO.NettoAverage30s*LIFTMODIFY, TEXT("Net Avg"));
    } else {
      RenderValue(orgTop.x, orgTop.y, &diValueTop, &diLabelTop, 
		  CALCULATED_INFO.Average30s*LIFTMODIFY, TEXT("Avg"));
    }
  }

  if (Appearance.GaugeVarioMc) {
    if (CALCULATED_INFO.AutoMacCready)
      RenderValue(orgBottom.x, orgBottom.y, 
		  &diValueBottom, &diLabelBottom, 
		  MACCREADY*LIFTMODIFY, TEXT("Auto Mc"));
    else
      RenderValue(orgBottom.x, orgBottom.y, 
		  &diValueBottom, &diLabelBottom, 
		  MACCREADY*LIFTMODIFY, TEXT("Mc"));
  }

  if (Appearance.GaugeVarioSpeedToFly) {
    RenderSpeedToFly(rc.right - 11, (rc.bottom-rc.top)/2);
  } else {
    RenderClimb();
  }

  if (Appearance.GaugeVarioBallast) {
    RenderBallast();
  }

  if (Appearance.GaugeVarioBugs) {
    RenderBugs();
  }

  dirty = false;
  RenderNeedle(vval);

  RenderValue(orgMiddle.x, orgMiddle.y, &diValueMiddle, &diLabelMiddle, 
	      vvaldisplay, 
	      TEXT("Gross"));

  BitBlt(hdcScreen, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);

}

void GaugeVario::Repaint(HDC hDC){
  BitBlt(hDC, 0, 0, rc.right, rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);
}

void GaugeVario::RenderBg() {
}

void GaugeVario::MakePolygon(const int i) {
  static bool InitDone = false;
  static int nlength0, nlength1, nwidth;
  double dx, dy;
  POINT *bit = getPolygon(i); 

  if (!InitDone){
    if (Appearance.GaugeVarioNeedleStyle == gvnsLongNeedle) {
      nlength0 = IBLSCALE(15); // was 18
      nlength1 = IBLSCALE(6);
      nwidth = IBLSCALE(4);  // was 3
    } else {
      nlength0 = IBLSCALE(13);
      nlength1 = IBLSCALE(6);
      nwidth = IBLSCALE(4);
    }    
    InitDone = true;
  }

#define ELLIPSE 1.06

  dx = -xoffset+nlength0; dy = nwidth;
  rotate(dx, dy, i);
  bit[0].x = lround(dx)+xoffset; bit[0].y = lround(dy*ELLIPSE)+yoffset+1;
  
  dx = -xoffset+nlength0; dy = -nwidth;
  rotate(dx, dy, i);
  bit[1].x = lround(dx)+xoffset; bit[1].y = lround(dy*ELLIPSE)+yoffset+1;
  
  dx = -xoffset+nlength1; dy = 0;
  rotate(dx, dy, i);
  bit[2].x = lround(dx)+xoffset; bit[2].y = lround(dy*ELLIPSE)+yoffset+1;
}


POINT *GaugeVario::getPolygon(int i) {
  return polys+(i+gmax)*3;
}

void GaugeVario::MakeAllPolygons() {
  polys = (POINT*)malloc((gmax*2+1)*3*sizeof(POINT));
  ASSERT(polys);
  if (polys) {
    for (int i= -gmax; i<= gmax; i++) {
      MakePolygon(i);
    }
  }
}


void GaugeVario::RenderClimb() {

  int x = rc.right-IBLSCALE(14);
  int y = rc.bottom-IBLSCALE(24);

  // testing  DrawInfo.SwitchState.VarioCircling = true;
  
  if (!dirty) return;

  if (!DrawInfo.SwitchState.VarioCircling) {
    if (Appearance.InverseInfoBox){
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    }
    SetBkMode(hdcDrawWindow, OPAQUE);

    Rectangle(hdcDrawWindow, 
	      x, y, 
	      x+IBLSCALE(12),y+IBLSCALE(12));
  } else {
    HBITMAP oldBmp = (HBITMAP)SelectObject(hdcTemp, hBitmapClimb);
    if (InfoBoxLayout::scale>1) {
      StretchBlt(hdcDrawWindow,
		 x,
		 y,
		 IBLSCALE(12), 
		 IBLSCALE(12),
		 hdcTemp,
		 12, 0,
		 12, 12,
		 SRCCOPY
		 );
    } else {
      BitBlt(hdcDrawWindow,
	     x, 
	     y,
	     12, 12,
	     hdcTemp,
	     12, 0,
	     SRCCOPY
	     );
    }
    SelectObject(hdcTemp, oldBmp);
  }
}

void GaugeVario::RenderNeedle(double Value){

  static int lastI = -99999;
  static int lastIv = -99999;
  static POINT lastBit[3];
  static POINT lp[2];
  static bool InitDone = false;
  static int degrees_per_unit;
  bool dirtytime = false;
  
  POINT *bit;
  int i;
  static DWORD fpsTimeLast =0;

  if (!InitDone){
    degrees_per_unit = 
      (int)((GAUGEVARIOSWEEP/2.0)/(GAUGEVARIORANGE*LIFTMODIFY));
    gmax = 
      max(80,(int)(degrees_per_unit*(GAUGEVARIORANGE*LIFTMODIFY))+2);
    MakeAllPolygons();
    InitDone = true;
  }

  i = iround(Value*degrees_per_unit*LIFTMODIFY);

  DWORD fpsTime = ::GetTickCount();
  if (fpsTime-fpsTimeLast>500) {
    //    if (i != lastIv) {
    dirty = true;
    fpsTimeLast = fpsTime;
    lastIv = i;
    //    }
  }

  i = min(gmax,max(-gmax,i));
  if ((i != lastI) || (i==gmax) || (i== -gmax)){

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

    bit = getPolygon(i);
    Polygon(hdcDrawWindow, bit, 3);
    lp[0].x = 0; lp[0].y = yoffset+1;
    lp[1].x = IBLSCALE(15); lp[1].y = yoffset+1;
    Polyline(hdcDrawWindow,lp,2);
    memcpy(lastBit, bit, 3*sizeof(POINT));

    lastI = i;
  }

}


// TODO: Optimise, this is slow
void GaugeVario::RenderValue(int x, int y, 
			     DrawInfo_t *diValue, 
			     DrawInfo_t *diLabel, double Value, 
			     TCHAR *Label) {

  SIZE tsize;

  Value = (double)iround(Value*10)/10;  // prevent the -0.0 case

  if (!diValue->InitDone){
    
    diValue->recBkg.right = x-IBLSCALE(5);
    diValue->recBkg.top = y +IBLSCALE(3)
      + Appearance.TitleWindowFont.CapitalHeight;
    
    diValue->recBkg.left = diValue->recBkg.right; 
    // update back rect with max label size
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
    diLabel->recBkg.top = y+IBLSCALE(1);

    diLabel->recBkg.left = diLabel->recBkg.right;           
    // update back rect with max label size
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

  if (dirty && (_tcscmp(diLabel->lastText, Label) != 0)) {
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

  if (dirty && (diValue->lastValue != Value)) {
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

  if (dirty && (diLabel->lastBitMap != hBitmapUnit)) {
    HBITMAP oldBmp;
    
    oldBmp = (HBITMAP)SelectObject(hdcTemp, hBitmapUnit);
    if (InfoBoxLayout::scale>1) {
      StretchBlt(hdcDrawWindow,
		 x-IBLSCALE(5), 
		 diValue->recBkg.top,
		 IBLSCALE(BitmapUnitSize.x), 
		 IBLSCALE(BitmapUnitSize.y),
		 hdcTemp,
		 BitmapUnitPos.x, BitmapUnitPos.y,
		 BitmapUnitSize.x, BitmapUnitSize.y,
		 SRCCOPY
		 );
    } else {
      BitBlt(hdcDrawWindow,
	     x-5, 
	     diValue->recBkg.top,
	     BitmapUnitSize.x, BitmapUnitSize.y,
	     hdcTemp,
	     BitmapUnitPos.x, BitmapUnitPos.y,
	     SRCCOPY
	     );
    }
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

  int nary = NARROWS*ARROWYSIZE;
  int ytop = rc.top
    +YOFFSET+nary; // JMW
  int ybottom = rc.bottom
    -YOFFSET-nary-InfoBoxLayout::scale; // JMW

  ytop += IBLSCALE(14);
  ybottom -= IBLSCALE(14);
  // JMW
  //  x = rc.left+IBLSCALE(1);
  x = rc.right-2*ARROWYSIZE-IBLSCALE(6);

  // only draw speed command if flying and vario is not circling
  // 
  if ((DerivedDrawInfo.Flying) 
#ifdef _SIM_
      && !DerivedDrawInfo.Circling  
#endif
      && !DrawInfo.SwitchState.VarioCircling) {
    vdiff = (DerivedDrawInfo.VOpt - DrawInfo.IndicatedAirspeed);
    vdiff = max(-DeltaVlimit, min(DeltaVlimit, vdiff)); // limit it
    vdiff = iround(vdiff/DeltaVstep) * DeltaVstep;
  } else
    vdiff = 0;

  if ((lastVdiff != vdiff)||(dirty)) {

    lastVdiff = vdiff;

    if (Appearance.InverseInfoBox){
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    }
    SetBkMode(hdcDrawWindow, OPAQUE);

    // ToDo sgi optimize

    // bottom (too slow)
    Rectangle(hdcDrawWindow, 
	      x, (ybottom+YOFFSET), 
	      x+ARROWXSIZE*2+1, (ybottom+YOFFSET)+nary+ARROWYSIZE+InfoBoxLayout::scale*2);

    // top (too fast)
    Rectangle(hdcDrawWindow, 
	      x, (ytop-YOFFSET)+1, 
	      x+ARROWXSIZE*2+1, (ytop-YOFFSET)-nary+1-ARROWYSIZE-InfoBoxLayout::scale*2);

    RenderClimb();

    if (Appearance.InverseInfoBox){
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
    } else {
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
      SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));
    }

    if (vdiff > 0){ // too slow

      y = ybottom;
      y += YOFFSET;

      while (vdiff > 0){
        if (vdiff > DeltaVstep){
          Rectangle(hdcDrawWindow, 
		    x, y, 
		    x+ARROWXSIZE*2+1, y+ARROWYSIZE-1);
        } else {
          POINT Arrow[4];
          Arrow[0].x = x; Arrow[0].y = y;
          Arrow[1].x = x+ARROWXSIZE; Arrow[1].y = y+ARROWYSIZE-1;
          Arrow[2].x = x+2*ARROWXSIZE; Arrow[2].y = y;
          Arrow[3].x = x; Arrow[3].y = y;
          Polygon(hdcDrawWindow, Arrow, 4);
        }
        vdiff -=DeltaVstep;
        y += ARROWYSIZE;
      }

    } else if (vdiff < 0){ // too fast

      y = ytop;
      y -= YOFFSET;

      while (vdiff < 0){
        if (vdiff < -DeltaVstep){
          Rectangle(hdcDrawWindow, x, y+1, 
		    x+ARROWXSIZE*2+1, y-ARROWYSIZE+2);
        } else {
          POINT Arrow[4];
          Arrow[0].x = x; Arrow[0].y = y;
          Arrow[1].x = x+ARROWXSIZE; Arrow[1].y = y-ARROWYSIZE+1;
          Arrow[2].x = x+2*ARROWXSIZE; Arrow[2].y = y;
          Arrow[3].x = x; Arrow[3].y = y;
          Polygon(hdcDrawWindow, Arrow, 4);
        }
        vdiff +=DeltaVstep;
        y -= ARROWYSIZE;
      }

    }

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
    orgLabel.y = rc.top + 2 + 
      (Appearance.TitleWindowFont.CapitalHeight*2) 
      - Appearance.TitleWindowFont.AscentHeight;

    orgValue.x = 1;                                         // position of ballast value
    orgValue.y = rc.top + 1 + 
      Appearance.TitleWindowFont.CapitalHeight 
      - Appearance.TitleWindowFont.AscentHeight;

    recLabelBk.left = orgLabel.x;                           // set upper left corner
    recLabelBk.top = orgLabel.y 
      + Appearance.TitleWindowFont.AscentHeight 
      - Appearance.TitleWindowFont.CapitalHeight;
    recValueBk.left = orgValue.x;                           // set upper left corner
    recValueBk.top = orgValue.y 
      + Appearance.TitleWindowFont.AscentHeight 
      - Appearance.TitleWindowFont.CapitalHeight;

    SelectObject(hdcDrawWindow, TitleWindowFont);           // get max label size
    GetTextExtentPoint(hdcDrawWindow, TextBal, _tcslen(TextBal), &tSize);

    recLabelBk.right = recLabelBk.left + tSize.cx;          // update back rect with max label size
    recLabelBk.bottom = recLabelBk.top + Appearance.TitleWindowFont.CapitalHeight;

                                                            // get max value size
    GetTextExtentPoint(hdcDrawWindow, TEXT("100%"), _tcslen(TEXT("100%")), &tSize);

    recValueBk.right = recValueBk.left + tSize.cx;          
     // update back rect with max label size
    recValueBk.bottom = recValueBk.top + Appearance.TitleWindowFont.CapitalHeight;

  }


  if (BALLAST != lastBallast){         
       // ballast hase been changed

    TCHAR Temp[18];

    SelectObject(hdcDrawWindow, TitleWindowFont);
    SetBkColor(hdcDrawWindow, colTextBackgnd);

    if (lastBallast < 0.001 || BALLAST < 0.001){
      if (BALLAST < 0.001)    // new ballast is 0, hide label
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TEXT(""), 0, NULL);
      else {
        SetTextColor(hdcDrawWindow, colTextGray); 
          // ols ballast was 0, show label
        ExtTextOut(hdcDrawWindow,
          orgLabel.x,
          orgLabel.y,
          ETO_OPAQUE, &recLabelBk, TextBal, _tcslen(TextBal), NULL);
      }
    }

    if (BALLAST < 0.001)         // new ballast 0, hide value
      _stprintf(Temp, TEXT(""));
    else
      _stprintf(Temp, TEXT("%.0f%%"), BALLAST*100);

    SetTextColor(hdcDrawWindow, colText);  // display value
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
    orgLabel.y = rc.bottom 
      - 2 - Appearance.TitleWindowFont.CapitalHeight - Appearance.TitleWindowFont.AscentHeight;

    orgValue.x = 1;
    orgValue.y = rc.bottom 
      - 1 - Appearance.TitleWindowFont.AscentHeight;

    recLabelBk.left = orgLabel.x;
    recLabelBk.top = orgLabel.y 
      + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;
    recValueBk.left = orgValue.x;
    recValueBk.top = orgValue.y 
      + Appearance.TitleWindowFont.AscentHeight - Appearance.TitleWindowFont.CapitalHeight;

    SelectObject(hdcDrawWindow, TitleWindowFont);
    GetTextExtentPoint(hdcDrawWindow, TextBug, _tcslen(TextBug), &tSize);

    recLabelBk.right = recLabelBk.left 
      + tSize.cx;
    recLabelBk.bottom = recLabelBk.top 
      + Appearance.TitleWindowFont.CapitalHeight + Appearance.TitleWindowFont.Height - Appearance.TitleWindowFont.AscentHeight;

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
      if (GlobalRunning && EnableVarioGauge) {
	hDC = BeginPaint(hwnd, &ps);
	GaugeVario::Repaint(hDC);
	DeleteDC(hDC);
	EndPaint(hwnd, &ps);
      }
    break;

  }

  return(DefWindowProc (hwnd, uMsg, wParam, lParam));
}



