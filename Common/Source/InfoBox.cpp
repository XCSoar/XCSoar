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
#include "Sizes.h"
#include "MapWindow.h"
#include "InfoBox.h"
#include "Dialogs.h"
#include "Utils.h"
#include "externs.h"
#include "InfoBoxLayout.h"

#define DEFAULTBORDERPENWIDTH 1
#define SELECTORWIDTH         (DEFAULTBORDERPENWIDTH+4)

extern HFONT  TitleWindowFont;
extern HFONT  MapWindowFont;
extern HFONT  MapWindowBoldFont;
extern HFONT  InfoWindowFont;
extern HFONT  CDIWindowFont;

//static ATOM atmWndClass;
static COLORREF redColor = RGB(0xff,0x0,0x0);
static COLORREF blueColor = RGB(0x0,0x0,0xff);
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

#if (NEWINFOBOX>0)

void InitInfoBoxModule(void);
LRESULT CALLBACK InfoBoxWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


InfoBox::InfoBox(HWND Parent, int X, int Y, int Width, int Height){
  mX = X;
  mY = Y;
  mWidth = Width;
  mHeight = Height;
  mParent = Parent;

  InitInfoBoxModule();

  mColorBack = bkColor;
  mColorFore = fgColor;
  color = 0;

  if (Count == 0){
    hBrushDefaultBackGround = (HBRUSH)CreateSolidBrush(bkColor);
    hBrushDefaultBackGroundSel = (HBRUSH)CreateSolidBrush(bkColorSel);
    hPenDefaultBorder = (HPEN)CreatePen(PS_SOLID, DEFAULTBORDERPENWIDTH, bdColor);
    hPenSelector = (HPEN)CreatePen(PS_SOLID, DEFAULTBORDERPENWIDTH+2, mColorFore);
  }

  mHWnd = CreateWindow(TEXT("STATIC"), TEXT("\0"),
		     WS_VISIBLE | WS_CHILD | SS_CENTER | SS_NOTIFY | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		     mX, mY,
		     mWidth, mHeight,
		     Parent, NULL, hInst, NULL);

  mVisible = false;

  mHdc = GetDC(mHWnd);
  mHdcTemp = CreateCompatibleDC(mHdc);

  SetWindowLong(mHWnd, GWL_WNDPROC, (LONG) InfoBoxWndProc);

  SetWindowLong(mHWnd, GWL_USERDATA, (long)this);

  mhBrushBk = hBrushDefaultBackGround;
  mhBrushBkSel = hBrushDefaultBackGroundSel;
  mhPenBorder = hPenDefaultBorder;
  mhPenSelector = hPenSelector;
  mBorderSize = 1;

  mBorderKind = BORDERRIGHT | BORDERBOTTOM;

  mphFontTitle   = &TitleWindowFont;
  mphFontValue   = &InfoWindowFont;
  mphFontComment = &TitleWindowFont;

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

  SetBkMode(mHdc, TRANSPARENT);

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
    PaintValue();

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
    Paint();
  }
  return(res);
}


void InfoBox::SetTitle(TCHAR *Value){
  TCHAR sTmp[TITLESIZE+1];

  _tcsncpy(sTmp, Value, TITLESIZE);
  sTmp[TITLESIZE] = '\0';

  if (Appearance.InfoTitelCapital)
    _tcsupr(sTmp);

  if (_tcscmp(mTitle, sTmp) != 0){
    _tcscpy(mTitle, sTmp);
    PaintTitle();
    PaintSelector();
  }
}

void InfoBox::SetValue(TCHAR *Value){
  if (_tcscmp(mValue, Value) != 0){
    _tcsncpy(mValue, Value, VALUESIZE);
    mValue[VALUESIZE] = '\0';
    PaintValue();
  }
}


void InfoBox::SetColor(int value) {
  if (Appearance.InfoBoxColors) {
    color = value;
  } else {
    color = 0;
  }
}

void InfoBox::SetComment(TCHAR *Value){
  if (_tcscmp(mComment, Value) != 0){
    _tcsncpy(mComment, Value, COMMENTSIZE);
    mComment[COMMENTSIZE] = '\0';
    PaintComment();
    PaintSelector();
  }
}

HWND InfoBox::GetHandle(void){
  return(mHWnd);
}

HWND InfoBox::GetParent(void){
  return(mParent);
}

void InfoBox::PaintTitle(void){

  SIZE tsize;
  int x,y;

  SetBkColor(mHdc, mColorTitleBk);

  SetTextColor(mHdc, mColorTitle);

  SelectObject(mHdc, *mphFontTitle);

  GetTextExtentPoint(mHdc, mTitle, _tcslen(mTitle), &tsize);

  x = recTitle.left + (mWidth - tsize.cx) / 2;
  if (x < 1)
    x = 1;

  if (mBorderKind & BORDERLEFT)
    x++;

  y = recTitle.top + 1 + mpFontHeightTitle->CapitalHeight - mpFontHeightTitle->AscentHeight;

  ExtTextOut(mHdc, x, y,
    ETO_OPAQUE, &recTitle, mTitle, _tcslen(mTitle), NULL);

}

void InfoBox::PaintValue(void){

  SIZE tsize;
  int x,y;

  SetBkColor(mHdc, mColorValueBk);

  switch (color) {
  case 0:
    SetTextColor(mHdc, mColorValue);
    break;
  case 1:
    SetTextColor(mHdc, redColor);
    break;
  case 2:
    SetTextColor(mHdc, blueColor);
    break;
  }

  SelectObject(mHdc, *mphFontValue);

  GetTextExtentPoint(mHdc, mValue, _tcslen(mValue), &tsize);

  x = recValue.left +
    (mWidth - tsize.cx - mBitmapUnitSize.x*InfoBoxLayout::scale) / 2;
  if (x < 1)
    x = 1;

  if (mBorderKind & BORDERLEFT)
    x++;

  y = recValue.top +
    (recValue.bottom - recValue.top+1)/2;
  y += ((mpFontHeightValue->CapitalHeight+1)/2)
    - mpFontHeightValue->AscentHeight;

  ExtTextOut(mHdc, x, y,
    ETO_OPAQUE, &recValue, mValue, _tcslen(mValue), NULL);

  if (mValueUnit != unUndef){
    if (mhBitmapUnit != NULL){
      HBITMAP oldBmp;
      oldBmp = (HBITMAP)SelectObject(mHdcTemp, mhBitmapUnit);
      if (InfoBoxLayout::scale>1) {
	StretchBlt(mHdc,
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
	BitBlt(mHdc,
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

void InfoBox::PaintComment(void){

  SIZE tsize;
  int x,y;

  SetBkColor(mHdc, mColorCommentBk);

  SetTextColor(mHdc, mColorComment);

  SelectObject(mHdc, *mphFontComment);

  GetTextExtentPoint(mHdc, mComment, _tcslen(mComment), &tsize);

  x = recComment.left + (mWidth - tsize.cx) / 2;
  if (x < 1)
    x = 1;

  if (mBorderKind & BORDERLEFT)
    x++;

  y = recComment.top + 1 + mpFontHeightComment->CapitalHeight - mpFontHeightComment->AscentHeight;

  ExtTextOut(mHdc, x, y,
    ETO_OPAQUE, &recComment, mComment, _tcslen(mComment), NULL);

}

void InfoBox::PaintSelector(void){

  if (mHasFocus){
    HPEN oldPen = (HPEN)SelectObject(mHdc, hPenSelector);

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

    SelectObject(mHdc,oldPen);
  }

}

void InfoBox::Paint(void){

  if (!GlobalRunning) return; // safety

  static bool InitDone = false;
  RECT rc;

  rc.left = 0;
  rc.top = 0;
  rc.right = 0 + mWidth;
  rc.bottom = 0 + mHeight;

  if (!InitDone){
    InitializeDrawHelpers();
    InitDone = false;
  }

  FillRect(mHdc, &rc, mhBrushBk);

  if (mBorderKind != 0){

    HPEN oldPen = (HPEN)SelectObject(mHdc, mhPenBorder);

    if (mBorderKind & BORDERTOP){
      MoveToEx(mHdc, 0, 0, NULL);
      LineTo(mHdc, mWidth, 0);
    }
    if (mBorderKind & BORDERRIGHT){
      MoveToEx(mHdc, mWidth-1, 0, NULL);
      LineTo(mHdc, mWidth-1, mHeight);
    }
    if (mBorderKind & BORDERBOTTOM){
      MoveToEx(mHdc, mWidth-1, mHeight-1, NULL);
      LineTo(mHdc, -1, mHeight-1);
    }
    if (mBorderKind & BORDERLEFT){
      MoveToEx(mHdc, 0, mHeight-1, NULL);
      LineTo(mHdc, 0, -1);
    }
    SelectObject(mHdc,oldPen);
  }

  PaintTitle();
  PaintComment();
  PaintValue();
  PaintSelector();

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


LRESULT CALLBACK InfoBoxWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
  InfoBox *ib;

  switch (uMsg){

    case WM_ERASEBKGND:
      /*
      ib = (InfoBox *)GetWindowLong(hwnd, GWL_USERDATA);
      if (ib)
	ib->Paint();
      */
    return TRUE;

    case WM_PAINT:
      ib = (InfoBox *)GetWindowLong(hwnd, GWL_USERDATA);
      if (ib)
	ib->Paint();
    break;

    case WM_SIZE:
    break;

    case WM_WINDOWPOSCHANGED:
      ib = (InfoBox *)GetWindowLong(hwnd, GWL_USERDATA);
      if (ib)
	ib->Paint();
    return 0;

    case WM_CREATE:
    break;

    case WM_DESTROY:
    break;

    case WM_LBUTTONDBLCLK:
    break;

    case WM_LBUTTONDOWN:
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

#endif
