/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

  $Id$
}
*/

#include "StdAfx.h"
#include "Defines.h" // VENTA3
#include "XCSoar.h"
#include "Sizes.h"
#include "MapWindow.h"
#include "InfoBox.h"
#include "Dialogs.h"
#include "Utils.h"
#include "externs.h"
#include "InfoBoxLayout.h"
#include "Compatibility/string.h"
#include "PeriodClock.hpp"
#include "WindowControls.h"

#define DEFAULTBORDERPENWIDTH IBLSCALE(1)
#define SELECTORWIDTH         (DEFAULTBORDERPENWIDTH+IBLSCALE(4))

extern HFONT  TitleWindowFont;
extern HFONT  TitleSmallWindowFont;
extern HFONT  MapWindowFont;
extern HFONT  MapWindowBoldFont;
extern HFONT  InfoWindowFont;
extern HFONT  CDIWindowFont;

//static ATOM atmWndClass;
COLORREF InfoBox::redColor = RGB(0xff,0x00,0x00);
COLORREF InfoBox::blueColor = RGB(0x00,0x00,0xff);
COLORREF InfoBox::inv_redColor = RGB(0xff,0x70,0x70);
COLORREF InfoBox::inv_blueColor = RGB(0x90,0x90,0xff);
COLORREF InfoBox::yellowColor = RGB(0xff,0xff,0x00);//VENTA2
COLORREF InfoBox::greenColor = RGB(0x00,0xff,0x00);//VENTA2
COLORREF InfoBox::magentaColor = RGB(0xff,0x00,0xff);//VENTA2
COLORREF InfoBox::inv_yellowColor = RGB(0xff,0xff,0x00); //VENTA2
COLORREF InfoBox::inv_greenColor = RGB(0x00,0xff,0x00); //VENTA2
COLORREF InfoBox::inv_magentaColor = RGB(0xff,0x00,0xff); //VENTA2


static COLORREF fgColor = RGB(0x0,0x0,0x0);
static COLORREF bkColor = RGB(0xff,0xff,0xff);
static COLORREF bkColorSel = RGB(150,0x0,0x0);
static COLORREF bdColor = RGB(80,80,80);
static DWORD lastErr;
static HBRUSH hBrushDefaultBackGround;
static HBRUSH hBrushDefaultBackGroundSel;
static HPEN hPenDefaultBorder;
static HPEN hPenSelector;
static int Count=0;


void InitInfoBoxModule(void);
LRESULT CALLBACK InfoBoxWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


InfoBox::InfoBox(HWND Parent, int X, int Y, int Width, int Height){
  mX = X;
  mY = Y;
  mWidth = Width;
  mHeight = Height;
  mParent = Parent;

  mTitleChanged = true;
  mSmallerFont = false;

  InitInfoBoxModule();

  mColorBack = bkColor;
  mColorFore = fgColor;
  color = 0;
  colorTop = 0;
  colorBottom = 0;

  if (Count == 0){
    hBrushDefaultBackGround = (HBRUSH)CreateSolidBrush(bkColor);
    hBrushDefaultBackGroundSel = (HBRUSH)CreateSolidBrush(bkColorSel);
    hPenDefaultBorder = (HPEN)CreatePen(PS_SOLID, DEFAULTBORDERPENWIDTH, bdColor);
    hPenSelector = (HPEN)CreatePen(PS_SOLID, DEFAULTBORDERPENWIDTH+2, mColorFore);
  }

  mHWnd = CreateWindow(TEXT("STATIC"), TEXT("\0"),
                     WS_VISIBLE | WS_CHILD | SS_CENTER | SS_NOTIFY
                       | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                     mX, mY,
                     mWidth, mHeight,
                     Parent, NULL, hInst, NULL);

  mVisible = false;

  mHdc = GetDC(mHWnd);
  mHdcTemp = CreateCompatibleDC(mHdc);

  // JMW added double buffering to reduce flicker
  mBufBitMap = CreateCompatibleBitmap(mHdc, mWidth, mHeight);
  mHdcBuf = CreateCompatibleDC(mHdc);
  SelectObject(mHdcBuf, mBufBitMap);

  SetWindowLong(mHWnd, GWL_WNDPROC, (LONG) InfoBoxWndProc);

  SetWindowLong(mHWnd, GWL_USERDATA, (long)this);

  mhBrushBk = hBrushDefaultBackGround;
  mhBrushBkSel = hBrushDefaultBackGroundSel;
  mhPenBorder = hPenDefaultBorder;
  mhPenSelector = hPenSelector;

  RECT rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = 0 + mWidth;
  rc.bottom = 0 + mHeight;
  FillRect(mHdcBuf, &rc, mhBrushBk);

  mBorderSize = 1;
  if (Appearance.InfoBoxBorder == apIbTab) {
    mBorderKind = BORDERTAB;
  } else {
    mBorderKind = BORDERRIGHT | BORDERBOTTOM;
  }

  mphFontTitle   = &TitleWindowFont;
  mphFontValue   = &InfoWindowFont;
  mphFontComment = &TitleWindowFont;
  valueFont	     = &TitleSmallWindowFont;

  mpFontHeightTitle = &Appearance.TitleWindowFont;
  mpFontHeightValue = &Appearance.InfoWindowFont;
  mpFontHeightComment = &Appearance.TitleWindowFont;

  if (Appearance.InverseInfoBox){
    mUnitBitmapKind = UNITBITMAPINVERS;
  } else {
    mUnitBitmapKind = UNITBITMAPNORMAL;
  }

  mColorTitle   = fgColor;
  mColorValue   = fgColor;
  mColorComment = fgColor;

  mColorTitleBk   = bkColor;
  mColorValueBk   = bkColor;
  mColorCommentBk = bkColor;

  mValueUnit = unUndef;

  _tcscpy(mTitle, TEXT(""));
  _tcscpy(mValue, TEXT(""));
  _tcscpy(mComment, TEXT(""));

  mBitmapUnitSize.x = 0;
  mBitmapUnitSize.y = 0;

  SetBkMode(mHdcBuf, TRANSPARENT);

  SetVisible(true);

  mHasFocus = false;

  Count++;

}

InfoBox::~InfoBox(void){

  Count--;

  if (Count==0){

    DeleteObject(hBrushDefaultBackGround);
    DeleteObject(hBrushDefaultBackGroundSel);
    DeleteObject(hPenDefaultBorder);
    DeleteObject(hPenSelector);

  }

  DeleteObject(mBufBitMap);
  DeleteDC(mHdcBuf);

  ReleaseDC(mHWnd, mHdc);
  DeleteDC(mHdcTemp);
  DestroyWindow(mHWnd);

}

void InfoBox::SetFocus(bool Value){

  if (mHasFocus != Value){
    mHasFocus = Value;
    Paint();
  }
}

bool InfoBox::SetVisible(bool Value){
  bool res = mVisible;
  if (mVisible != Value){
    mVisible = Value;
    if (mVisible)
      ShowWindow(mHWnd, SW_SHOW);
    else
      ShowWindow(mHWnd, SW_HIDE);
  }
  return(res);
}

Units_t InfoBox::SetValueUnit(Units_t Value){
  Units_t res = mValueUnit;
  if (mValueUnit != Value){
    mValueUnit = Value;

    Units::GetUnitBitmap(mValueUnit, &mhBitmapUnit,
                         &mBitmapUnitPos,
                         &mBitmapUnitSize, mUnitBitmapKind);
    //JMW    PaintValue();

  }
  return(res);
}

int InfoBox::GetBorderKind(void){
  return(mBorderKind);
}

int InfoBox::SetBorderKind(int Value){
  int res = mBorderKind;
  if (mBorderKind != Value){
    mBorderKind = Value;

    if (Appearance.InfoBoxBorder == apIbTab) {
      mBorderKind = BORDERTAB;
    } else {
      mBorderKind = Value;
    }
    //JMW    Paint();
  }
  return(res);
}


void InfoBox::SetTitle(const TCHAR *Value){
  TCHAR sTmp[TITLESIZE+1];

  _tcsncpy(sTmp, Value, TITLESIZE);
  sTmp[TITLESIZE] = '\0';

  if (Appearance.InfoTitelCapital)
    _tcsupr(sTmp);

  if (_tcscmp(mTitle, sTmp) != 0){
    _tcscpy(mTitle, sTmp);
    mTitleChanged = true;
    //JMW    PaintTitle();
    //JMW    PaintSelector();
  }
}

void InfoBox::SetValue(const TCHAR *Value){
  if (_tcscmp(mValue, Value) != 0){
    _tcsncpy(mValue, Value, VALUESIZE);
    mValue[VALUESIZE] = '\0';
    //JMW    PaintValue();
  }
}


void InfoBox::SetColor(int value) {
  if (Appearance.InfoBoxColors) {
    color = value;
  } else {
    color = 0;
  }
}

void InfoBox::SetColorBottom(int value) {
  if (Appearance.InfoBoxColors) {
    colorBottom = value;
  } else {
    colorBottom = 0;
  }
}

void InfoBox::SetColorTop(int value) {
  if (Appearance.InfoBoxColors) {
    colorTop = value;
  } else {
    colorTop = 0;
  }
}
void InfoBox::SetComment(const TCHAR *Value){
  if (_tcscmp(mComment, Value) != 0){
    _tcsncpy(mComment, Value, COMMENTSIZE);
    mComment[COMMENTSIZE] = '\0';
    //JMW    PaintComment();
    //JMW    PaintSelector();
  }
}

HWND InfoBox::GetHandle(void){
  return(mHWnd);
}

HWND InfoBox::GetParent(void){
  return(mParent);
}

void InfoBox::SetSmallerFont(bool smallerFont)
{
	this->mSmallerFont = smallerFont;
}

void InfoBox::PaintTitle(HDC mHdcBuf){

  if (!mTitleChanged) return;

  SIZE tsize;
  int x,y;
  int halftextwidth;

  SetBkColor(mHdcBuf, mColorTitleBk);
 // SetTextColor(mHdcBuf, mColorTitle);
  switch (colorTop) {
  case -1:
    SetTextColor(mHdcBuf, bdColor);
    break;
  case 0:
    SetTextColor(mHdcBuf, mColorValue);
    break;
  case 1:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_redColor);
    } else {
      SetTextColor(mHdcBuf, redColor);
    }
    break;
  case 2:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_blueColor);
    } else {
      SetTextColor(mHdcBuf, blueColor);
    }
    break;
// VENTA3 added colors
  case 3:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_greenColor);
    } else {
      SetTextColor(mHdcBuf, greenColor);
    }
    break;
  case 4:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_yellowColor);
    } else {
      SetTextColor(mHdcBuf, yellowColor);
    }
    break;
  case 5:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_magentaColor);
    } else {
      SetTextColor(mHdcBuf, magentaColor);
    }
    break;
  }

  SelectObject(mHdcBuf, *mphFontTitle);

  GetTextExtentPoint(mHdcBuf, mTitle, _tcslen(mTitle), &tsize);

  halftextwidth = (mWidth - tsize.cx)>>1;

  x = max(1,recTitle.left + halftextwidth);

  y = recTitle.top + 1 + mpFontHeightTitle->CapitalHeight
    - mpFontHeightTitle->AscentHeight;

  if (mBorderKind & BORDERLEFT)
    x+= DEFAULTBORDERPENWIDTH;

  ExtTextOut(mHdcBuf, x, y,
    ETO_OPAQUE, &recTitle, mTitle, _tcslen(mTitle), NULL);

  if ((mBorderKind & BORDERTAB) && (halftextwidth>IBLSCALE(3))) {

    int ytop = recTitle.top + (mpFontHeightTitle->CapitalHeight)/2;
    int ytopedge = ytop+IBLSCALE(2);
    int ybottom = recTitle.top + IBLSCALE(6) + mpFontHeightTitle->CapitalHeight;

    HPEN oldPen = (HPEN)SelectObject(mHdcBuf, mhPenBorder);

    POINT tab[8];
    tab[0].x = tab[1].x = recTitle.left+IBLSCALE(1);
    tab[0].y = tab[7].y = ybottom;
    tab[2].x = recTitle.left+IBLSCALE(3);
    tab[2].y = tab[5].y = tab[3].y = tab[4].y = ytop;
    tab[1].y = tab[6].y = ytopedge;
    tab[5].x = recTitle.right-IBLSCALE(4);
    tab[6].x = tab[7].x = recTitle.right-IBLSCALE(2);
    tab[3].x = recTitle.left+halftextwidth-IBLSCALE(1);
    tab[4].x = recTitle.right-halftextwidth+IBLSCALE(1);

    Polyline(mHdcBuf, tab, 4);
    Polyline(mHdcBuf, tab+4, 4);

    SelectObject(mHdcBuf,oldPen);

  }

  mTitleChanged = false;

}

void InfoBox::PaintValue(HDC mHdcBuf){

  SIZE tsize;
  int x,y;
  unsigned int len = _tcslen(mValue);

  SetBkColor(mHdcBuf, mColorValueBk);

  switch (color) {
  case -1:
    SetTextColor(mHdcBuf, bdColor);
    break;
  case 0:
    SetTextColor(mHdcBuf, mColorValue);
    break;
  case 1:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_redColor);
    } else {
      SetTextColor(mHdcBuf, redColor);
    }
    break;
  case 2:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_blueColor);
    } else {
      SetTextColor(mHdcBuf, blueColor);
    }
    break;
// VENTA3 more colors
  case 3:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_greenColor);
    } else {
      SetTextColor(mHdcBuf, greenColor);
    }
    break;
  case 4:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_yellowColor);
    } else {
      SetTextColor(mHdcBuf, yellowColor);
    }
    break;
  case 5:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_magentaColor);
    } else {
      SetTextColor(mHdcBuf, magentaColor);
    }
    break;
  }

  if (mSmallerFont)
  {
    SelectObject(mHdcBuf, *valueFont);
  }
  else
  {
    SelectObject(mHdcBuf, *mphFontValue);
  }

  GetTextExtentPoint(mHdcBuf, mValue, len, &tsize);

  x = max(1,recValue.left +
          (mWidth - tsize.cx - mBitmapUnitSize.x*InfoBoxLayout::scale) / 2);

  if (mBorderKind & BORDERLEFT)
    x+= DEFAULTBORDERPENWIDTH;

  y = recValue.top + 1 - mpFontHeightValue->AscentHeight +
    (recValue.bottom - recValue.top + mpFontHeightValue->CapitalHeight)/2;

  ExtTextOut(mHdcBuf, x, y, ETO_OPAQUE, &recValue, mValue, len, NULL);

  if ((mValueUnit != unUndef) && (color>=0)){
    if (mhBitmapUnit != NULL){
      HBITMAP oldBmp;
      oldBmp = (HBITMAP)SelectObject(mHdcTemp, mhBitmapUnit);
      if (InfoBoxLayout::scale>1) {
        StretchBlt(mHdcBuf,
                   x+tsize.cx,
                   y+mpFontHeightValue->AscentHeight
                   -mBitmapUnitSize.y*InfoBoxLayout::scale,
                   mBitmapUnitSize.x*InfoBoxLayout::scale,
                   mBitmapUnitSize.y*InfoBoxLayout::scale,
                   mHdcTemp,
                   mBitmapUnitPos.x, mBitmapUnitPos.y,
                   mBitmapUnitSize.x,
                   mBitmapUnitSize.y,
               SRCCOPY
               );
      } else {
        BitBlt(mHdcBuf,
               x+tsize.cx,
               y+mpFontHeightValue->AscentHeight
               -mBitmapUnitSize.y,
               mBitmapUnitSize.x, mBitmapUnitSize.y,
               mHdcTemp,
               mBitmapUnitPos.x, mBitmapUnitPos.y,
               SRCCOPY
               );
      }
      SelectObject(mHdcTemp ,oldBmp);
    }
  }

}

void InfoBox::PaintComment(HDC mHdcBuf){

  SIZE tsize;
  int x,y;
  unsigned int len = _tcslen(mComment);

  if (len==0) return; // nothing to paint


  switch (colorBottom) {
  case -1:
    SetTextColor(mHdcBuf, bdColor);
    break;
  case 0:
    SetTextColor(mHdcBuf, mColorValue);
    break;
  case 1:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_redColor);
    } else {
      SetTextColor(mHdcBuf, redColor);
    }
    break;
  case 2:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_blueColor);
    } else {
      SetTextColor(mHdcBuf, blueColor);
    }
    break;
// VENTA3 more colors
  case 3:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_greenColor);
    } else {
      SetTextColor(mHdcBuf, greenColor);
    }
    break;
  case 4:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_yellowColor);
    } else {
      SetTextColor(mHdcBuf, yellowColor);
    }
    break;
  case 5:
    if (Appearance.InverseInfoBox){
      SetTextColor(mHdcBuf, inv_magentaColor);
    } else {
      SetTextColor(mHdcBuf, magentaColor);
    }
    break;
  }


  SetBkColor(mHdcBuf, mColorCommentBk);

 // SetTextColor(mHdcBuf, mColorComment);

  SelectObject(mHdcBuf, *mphFontComment);

  GetTextExtentPoint(mHdcBuf, mComment, len, &tsize);

  x = max(1,recComment.left + (mWidth - tsize.cx) / 2);
  if (mBorderKind & BORDERLEFT)
    x+= DEFAULTBORDERPENWIDTH;

  y = recComment.top + 1
    + mpFontHeightComment->CapitalHeight
    - mpFontHeightComment->AscentHeight;

  ExtTextOut(mHdcBuf, x, y,
    ETO_OPAQUE, &recComment, mComment, len, NULL);

}


void InfoBox::PaintSelector(HDC mHdc){

  if (mHasFocus){
    HPEN oldPen = (HPEN)SelectObject(mHdc, hPenSelector);

#ifndef NOLINETO

    MoveToEx(mHdc, mWidth-SELECTORWIDTH-1, 0, NULL);
    LineTo(mHdc, mWidth-1, 0);
    LineTo(mHdc, mWidth-1, SELECTORWIDTH+1);

    MoveToEx(mHdc, mWidth-1, mHeight-SELECTORWIDTH-2, NULL);
    LineTo(mHdc, mWidth-1, mHeight-1);
    LineTo(mHdc, mWidth-SELECTORWIDTH-1, mHeight-1);

    MoveToEx(mHdc, SELECTORWIDTH+1, mHeight-1, NULL);
    LineTo(mHdc, 0, mHeight-1);
    LineTo(mHdc, 0, mHeight-SELECTORWIDTH-2);

    MoveToEx(mHdc, 0, SELECTORWIDTH+1, NULL);
    LineTo(mHdc, 0, 0);
    LineTo(mHdc, SELECTORWIDTH+1, 0);

#else
    POINT p[2];
    p[0].x = mWidth-SELECTORWIDTH-1;
    p[0].y = 0;
    p[1].x = mWidth-1;
    p[1].y = 0;
    Polyline(mHdc, p, 2);
    p[0].x = mWidth-1;
    p[0].y = SELECTORWIDTH+1;
    Polyline(mHdc, p, 2);

    p[0].x = mWidth-1;
    p[0].y = mHeight-SELECTORWIDTH-2;
    p[1].x = mWidth-1;
    p[1].y = mHeight-1;
    Polyline(mHdc, p, 2);
    p[0].x = mWidth-SELECTORWIDTH-1;
    p[0].y = mHeight-1;
    Polyline(mHdc, p, 2);

    p[0].x = SELECTORWIDTH+1;
    p[0].y = mHeight-1;
    p[1].x = 0;
    p[1].y = mHeight-1;
    Polyline(mHdc, p, 2);
    p[0].x = 0;
    p[0].y = mHeight-SELECTORWIDTH-2;
    Polyline(mHdc, p, 2);

    p[0].x = 0;
    p[0].y = SELECTORWIDTH+1;
    p[1].x = 0;
    p[1].y = 0;
    Polyline(mHdc, p, 2);
    p[0].x = SELECTORWIDTH+1;
    p[0].y = 0;
    Polyline(mHdc, p, 2);

#endif

    SelectObject(mHdc,oldPen);
  }

}

void InfoBox::Paint(){

  if (!GlobalRunning) return; // safety

  static bool InitDone = false;
  RECT rc;

  if (!InitDone){
    InitializeDrawHelpers();
    InitDone = false;
  }

  if (mTitleChanged) {
    // clear the whole area
    rc.left = 0;
    rc.top = 0;
    rc.right = 0 + mWidth;
    rc.bottom = 0 + mHeight;
  } else {
    rc.left = 0;
    rc.top = recTitle.bottom;
    rc.right = 0 + mWidth;
    rc.bottom = 0 + mHeight;
  }
  FillRect(mHdcBuf, &rc, mhBrushBk);

  if (mBorderKind != 0){

    HPEN oldPen = (HPEN)SelectObject(mHdcBuf, mhPenBorder);
#ifndef NOLINETO

    if (mBorderKind & BORDERTOP){
      MoveToEx(mHdcBuf, 0, 0, NULL);
      LineTo(mHdcBuf, mWidth, 0);
    }
    if (mBorderKind & BORDERRIGHT){
      MoveToEx(mHdcBuf, mWidth-DEFAULTBORDERPENWIDTH, 0, NULL);
      LineTo(mHdcBuf, mWidth-DEFAULTBORDERPENWIDTH, mHeight);
    }
    if (mBorderKind & BORDERBOTTOM){
      MoveToEx(mHdcBuf, mWidth-DEFAULTBORDERPENWIDTH, mHeight-DEFAULTBORDERPENWIDTH, NULL);
      LineTo(mHdcBuf, -DEFAULTBORDERPENWIDTH, mHeight-DEFAULTBORDERPENWIDTH);
    }
    if (mBorderKind & BORDERLEFT){
      MoveToEx(mHdcBuf, 0, mHeight-DEFAULTBORDERPENWIDTH, NULL);
      LineTo(mHdcBuf, 0, -DEFAULTBORDERPENWIDTH);
    }
#else
    POINT p[2];
    if (mBorderKind & BORDERTOP){
      p[0].x = 0; p[0].y= 0;
      p[1].x = mWidth; p[1].y= 0;
      Polyline(mHdcBuf, p, 2);
    }
    if (mBorderKind & BORDERRIGHT){
      p[0].x = mWidth-DEFAULTBORDERPENWIDTH; p[0].y= 0;
      p[1].x = mWidth-DEFAULTBORDERPENWIDTH; p[1].y= mHeight;
      Polyline(mHdcBuf, p, 2);
    }
    if (mBorderKind & BORDERBOTTOM){
      p[0].x = mWidth-DEFAULTBORDERPENWIDTH; p[0].y= mHeight-DEFAULTBORDERPENWIDTH;
      p[1].x = -DEFAULTBORDERPENWIDTH; p[1].y= mHeight-DEFAULTBORDERPENWIDTH;
      Polyline(mHdcBuf, p, 2);
    }
    if (mBorderKind & BORDERLEFT){
      p[0].x = 0; p[0].y= mHeight-DEFAULTBORDERPENWIDTH;
      p[1].x = 0; p[1].y= -DEFAULTBORDERPENWIDTH;
      Polyline(mHdcBuf, p, 2);
    }
#endif

    SelectObject(mHdcBuf,oldPen);
  }

  PaintTitle(mHdcBuf);
  PaintComment(mHdcBuf);
  PaintValue(mHdcBuf);
}


void InfoBox::PaintFast(void) {
  BitBlt(mHdc, 0, 0, mWidth, mHeight,
         mHdcBuf, 0, 0, SRCCOPY);
  PaintSelector(mHdc);
}


void InfoBox::PaintInto(HDC mHdcDest, int xoff, int yoff, int width, int height) {
  StretchBlt(mHdcDest, xoff, yoff, width, height,
          mHdcBuf, 0, 0, mWidth, mHeight, SRCCOPY);
}

HDC InfoBox::GetHdcBuf(void) {
  return mHdcBuf;
}

void InfoBox::InitializeDrawHelpers(void){

  recTitle.left = 0;
  recTitle.right = mWidth;
  recTitle.top = 0;
  recTitle.bottom = mpFontHeightTitle->CapitalHeight + 2;

  recComment.left = 0;
  recComment.right = mWidth;
  recComment.bottom = mHeight;
  recComment.top = recComment.bottom - (mpFontHeightTitle->CapitalHeight + 2);

  recValue.left = 0;
  recValue.right = mWidth;
  recValue.top = recTitle.bottom;
  recValue.bottom = recComment.top;

  if (mBorderKind & BORDERLEFT){
    recTitle.left += mBorderSize;
    recValue.left += mBorderSize;
    recComment.left += mBorderSize;
  }

  if (mBorderKind & BORDERRIGHT){
    recTitle.right -= mBorderSize;
    recValue.right -= mBorderSize;
    recComment.right -= mBorderSize;
  }

  if (mBorderKind & BORDERTOP){
    recTitle.top += mBorderSize;
    recTitle.bottom += mBorderSize;
    recValue.top += mBorderSize;
  }

  if (mBorderKind & BORDERBOTTOM){
    recValue.bottom -= mBorderSize;
    recComment.top -= mBorderSize;
    recComment.bottom -= mBorderSize;
  }

}

extern void ShowMenu();

LRESULT CALLBACK InfoBoxWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
  InfoBox *ib;
  static PeriodClock double_click;

  switch (uMsg){

    case WM_ERASEBKGND:
      /* JMW we erase ourselves so can prevent flicker by eliminating
         windows from doing it.
      ib = (InfoBox *)GetWindowLong(hwnd, GWL_USERDATA);
      if (ib)
        ib->Paint();
      */
    return TRUE;

    case WM_PAINT:
      ib = (InfoBox *)GetWindowLong(hwnd, GWL_USERDATA);
      if (ib)
        ib->PaintFast();

    break;

    case WM_SIZE:
    break;

    case WM_WINDOWPOSCHANGED:
      ib = (InfoBox *)GetWindowLong(hwnd, GWL_USERDATA);
      if (ib)
        ib->PaintFast();
    return 0;

    case WM_CREATE:
    break;

    case WM_DESTROY:
    break;

    case WM_LBUTTONDBLCLK:
#ifndef GNAV
      // JMW capture double click, so infoboxes double clicked also bring up menu
      // VENTA3: apparently this is working only on PC ! Disable it to let PC work
      // with same timeout of PDA and PNA versions with synthetic DBLCLK
#ifdef DEBUG_DBLCLK
      DoStatusMessage(_T("DBLCLK InfoBox")); // VENTA3
#endif
      ShowMenu();
      break;
#endif

    case WM_LBUTTONDOWN:
      /*
       * VENTA3 SYNTHETIC DOUBLE CLICK ON INFOBOXES
       * Paolo Ventafridda
       * synthetic double click detection with no proximity , good for infoboxes
       */

        if (!double_click.check_always_update(DOUBLECLICKINTERVAL)) {
#ifdef DEBUG_DBLCLK
	  DoStatusMessage(_T("synth DBLCLK InfoBox!")); // VENTA3
#endif
	  ShowMenu();
	  return(0);
	}
#ifdef DEBUG_DBLCLK
        DoStatusMessage(_T("BDOWN InfoBox")); // VENTA3
#endif
        ib = (InfoBox *)GetWindowLong(hwnd, GWL_USERDATA);
        if (ib)
	  SendMessage(ib->GetParent(), WM_COMMAND, (WPARAM)0, (LPARAM)ib->GetHandle());
    return(0);

    case WM_LBUTTONUP:
    break;

    case WM_KEYUP:
    break;
  }

  return (DefWindowProc (hwnd, uMsg, wParam, lParam));
}


void InitInfoBoxModule(void){

  static bool InitDone = false;

  if (InitDone)
    return;

  if (Appearance.InverseInfoBox){
    COLORREF cl;
    cl = fgColor;
    fgColor = bkColor;
    bkColor = cl;
  }

  InitDone = true;
}

