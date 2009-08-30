/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "InfoBox.h"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Dialogs.h"
#include "Utils.h"
#include "InfoBoxLayout.h"
#include "InputEvents.h"
#include "Compatibility/string.h"
#include "PeriodClock.hpp"
#include "WindowControls.h"
#include "Interface.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/BitmapCanvas.hpp"

static Color fgColor = RGB(0x0,0x0,0x0);
static Color bkColor = RGB(0xff,0xff,0xff);
static Color bkColorSel = RGB(150,0x0,0x0);
static Color bdColor = RGB(80,80,80);
static Brush hBrushDefaultBackGround;
static Brush hBrushDefaultBackGroundSel;
static Pen hPenDefaultBorder;
static Pen hPenSelector;
static int Count=0;


// infobox
#define DEFAULTBORDERPENWIDTH IBLSCALE(1)
#define SELECTORWIDTH         (DEFAULTBORDERPENWIDTH+IBLSCALE(4))


void InitInfoBoxModule(void);

InfoBox::InfoBox(ContainerWindow &parent, int X, int Y, int Width, int Height)
  :mParent(parent)
{
  mX = X;
  mY = Y;
  mWidth = Width;
  mHeight = Height;

  mTitleChanged = true;
  mSmallerFont = false;

  InitInfoBoxModule();

  color = 0;
  colorTop = 0;
  colorBottom = 0;

  set(parent, mX, mY, mWidth, mHeight, true, true);

  mVisible = false;

  Color fgColor, bkColor, bkColorSel;

  if (Appearance.InverseInfoBox){
    fgColor = Color(0xff, 0xff, 0xff);
    bkColor = Color(0x00, 0x00, 0x00);
  } else {
    bkColor = Color(0xff, 0xff, 0xff);
    fgColor = Color(0x00, 0x00, 0x00);
  }

  bkColorSel = Color(150, 0x0, 0x0);
  bdColor = Color(80, 80, 80);

  mColorBack = bkColor;
  mColorFore = fgColor;

  if (Count == 0){
    hBrushDefaultBackGround.set(bkColor);
    hBrushDefaultBackGroundSel.set(bkColorSel);
    hPenDefaultBorder.set(DEFAULTBORDERPENWIDTH, bdColor);
    hPenSelector.set(DEFAULTBORDERPENWIDTH + 2, mColorFore);
  }

  // JMW added double buffering to reduce flicker
  buffer.set(get_canvas(), mWidth, mHeight);

  install_wndproc();

  mhBrushBk = hBrushDefaultBackGround;
  mhBrushBkSel = hBrushDefaultBackGroundSel;
  mhPenBorder = hPenDefaultBorder;
  mhPenSelector = hPenSelector;

  RECT rc;
  rc.left = 0;
  rc.top = 0;
  rc.right = 0 + mWidth;
  rc.bottom = 0 + mHeight;
  buffer.fill_rectangle(0, 0, mWidth, mHeight, mhBrushBk);

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

  buffer.background_transparent();

  SetVisible(true);

  mHasFocus = false;

  Count++;

}

InfoBox::~InfoBox(void){

  Count--;

  if (Count==0){

    hBrushDefaultBackGround.reset();
    hBrushDefaultBackGroundSel.reset();
    hPenDefaultBorder.reset();
    hPenSelector.reset();

  }

  buffer.reset();
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
      show();
    else
      hide();
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

PaintWindow &InfoBox::GetHandle(void){
  return *this;
}

ContainerWindow &InfoBox::GetParent(void){
  return(mParent);
}

void InfoBox::SetSmallerFont(bool smallerFont)
{
	this->mSmallerFont = smallerFont;
}

void InfoBox::PaintTitle(Canvas &canvas){

  if (!mTitleChanged) return;

  SIZE tsize;
  int x,y;
  int halftextwidth;

  canvas.set_background_color(mColorTitleBk);
 // SetTextColor(mHdcBuf, mColorTitle);
  switch (colorTop) {
  case -1:
    canvas.set_text_color(bdColor);
    break;
  case 0:
    canvas.set_text_color(mColorValue);
    break;
  case 1:
    canvas.set_text_color(Appearance.InverseInfoBox
                          ? MapGfx.inv_redColor : MapGfx.redColor);
    break;
  case 2:
    canvas.set_text_color(Appearance.InverseInfoBox
                          ? MapGfx.inv_blueColor : MapGfx.blueColor);
    break;
// VENTA3 added colors
  case 3:
    canvas.set_text_color(Appearance.InverseInfoBox
                          ? MapGfx.inv_greenColor : MapGfx.greenColor);
    break;
  case 4:
    canvas.set_text_color(Appearance.InverseInfoBox
                          ? MapGfx.inv_yellowColor : MapGfx.yellowColor);
    break;
  case 5:
    canvas.set_text_color(Appearance.InverseInfoBox
                          ? MapGfx.inv_magentaColor : MapGfx.magentaColor);
    break;
  }

  canvas.select(*mphFontTitle);

  tsize = canvas.text_size(mTitle);

  halftextwidth = (mWidth - tsize.cx)>>1;

  x = max(1,recTitle.left + halftextwidth);

  y = recTitle.top + 1 + mpFontHeightTitle->CapitalHeight
    - mpFontHeightTitle->AscentHeight;

  if (mBorderKind & BORDERLEFT)
    x+= DEFAULTBORDERPENWIDTH;

  canvas.text_opaque(x, y, &recTitle, mTitle);

  if ((mBorderKind & BORDERTAB) && (halftextwidth>IBLSCALE(3))) {

    int ytop = recTitle.top + (mpFontHeightTitle->CapitalHeight)/2;
    int ytopedge = ytop+IBLSCALE(2);
    int ybottom = recTitle.top + IBLSCALE(6) + mpFontHeightTitle->CapitalHeight;

    canvas.select(mhPenBorder);

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

    canvas.polyline(tab, 4);
    canvas.polyline(tab + 4, 4);
  }

  mTitleChanged = false;

}

void InfoBox::PaintValue(Canvas &canvas){

  SIZE tsize;
  int x,y;
  unsigned int len = _tcslen(mValue);

  canvas.set_background_color(mColorValueBk);

  switch (color) {
  case -1:
    canvas.set_text_color(bdColor);
    break;
  case 0:
    canvas.set_text_color(mColorValue);
    break;
  case 1:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_redColor);
    } else {
      canvas.set_text_color(MapGfx.redColor);
    }
    break;
  case 2:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_blueColor);
    } else {
      canvas.set_text_color(MapGfx.blueColor);
    }
    break;
// VENTA3 more colors
  case 3:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_greenColor);
    } else {
      canvas.set_text_color(MapGfx.greenColor);
    }
    break;
  case 4:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_yellowColor);
    } else {
      canvas.set_text_color(MapGfx.yellowColor);
    }
    break;
  case 5:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_magentaColor);
    } else {
      canvas.set_text_color(MapGfx.magentaColor);
    }
    break;
  }

  canvas.select(mSmallerFont ? *valueFont : *mphFontValue);

  tsize = canvas.text_size(mValue);

  x = max(1,recValue.left +
          (mWidth - tsize.cx - mBitmapUnitSize.x*InfoBoxLayout::scale) / 2);

  if (mBorderKind & BORDERLEFT)
    x+= DEFAULTBORDERPENWIDTH;

  y = recValue.top + 1 - mpFontHeightValue->AscentHeight +
    (recValue.bottom - recValue.top + mpFontHeightValue->CapitalHeight)/2;

  canvas.text_opaque(x, y, &recValue, mValue);

  if ((mValueUnit != unUndef) && (color>=0)){
    if (mhBitmapUnit != NULL){
      BitmapCanvas temp(canvas, *mhBitmapUnit);
      canvas.scale_copy(x + tsize.cx,
                        y + mpFontHeightValue->AscentHeight
                        - mBitmapUnitSize.y * InfoBoxLayout::scale,
                        temp,
                        mBitmapUnitPos.x, mBitmapUnitPos.y,
                        mBitmapUnitSize.x, mBitmapUnitSize.y);
    }
  }

}

void InfoBox::PaintComment(Canvas &canvas){

  SIZE tsize;
  int x,y;
  unsigned int len = _tcslen(mComment);

  if (len==0) return; // nothing to paint


  switch (colorBottom) {
  case -1:
    canvas.set_text_color(bdColor);
    break;
  case 0:
    canvas.set_text_color(mColorValue);
    break;
  case 1:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_redColor);
    } else {
      canvas.set_text_color(MapGfx.redColor);
    }
    break;
  case 2:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_blueColor);
    } else {
      canvas.set_text_color(MapGfx.blueColor);
    }
    break;
// VENTA3 more colors
  case 3:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_greenColor);
    } else {
      canvas.set_text_color(MapGfx.greenColor);
    }
    break;
  case 4:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_yellowColor);
    } else {
      canvas.set_text_color(MapGfx.yellowColor);
    }
    break;
  case 5:
    if (Appearance.InverseInfoBox){
      canvas.set_text_color(MapGfx.inv_magentaColor);
    } else {
      canvas.set_text_color(MapGfx.magentaColor);
    }
    break;
  }


  canvas.set_background_color(mColorCommentBk);

 // SetTextColor(mHdcBuf, mColorComment);

  canvas.select(*mphFontComment);

  tsize = canvas.text_size(mComment);

  x = max(1,recComment.left + (mWidth - tsize.cx) / 2);
  if (mBorderKind & BORDERLEFT)
    x+= DEFAULTBORDERPENWIDTH;

  y = recComment.top + 1
    + mpFontHeightComment->CapitalHeight
    - mpFontHeightComment->AscentHeight;

  canvas.text_opaque(x, y, &recComment, mComment);

}


void InfoBox::PaintSelector(Canvas &canvas){

  if (mHasFocus){
    canvas.select(hPenSelector);

    canvas.two_lines(mWidth - SELECTORWIDTH - 1, 0,
                     mWidth - 1, 0,
                     mWidth - 1, SELECTORWIDTH + 1);

    canvas.two_lines(mWidth - 1, mHeight - SELECTORWIDTH - 2,
                     mWidth - 1, mHeight - 1,
                     mWidth - SELECTORWIDTH - 1, mHeight - 1);

    canvas.two_lines(SELECTORWIDTH + 1, mHeight - 1,
                     0, mHeight - 1,
                     0, mHeight - SELECTORWIDTH - 2);

    canvas.two_lines(0, SELECTORWIDTH + 1,
                     0, 0,
                     SELECTORWIDTH + 1, 0);
  }

}

void InfoBox::Paint(){

  if (!globalRunningEvent.test()) return; // safety

  static bool InitDone = false;

  if (!InitDone){
    InitializeDrawHelpers();
    InitDone = false;
  }

  buffer.fill_rectangle(0, mTitleChanged ? 0 : recTitle.bottom,
                        mWidth, mHeight, mhBrushBk);

  if (mBorderKind != 0){
    buffer.select(mhPenBorder);

    if (mBorderKind & BORDERTOP){
      buffer.line(0, 0, mWidth, 0);
    }
    if (mBorderKind & BORDERRIGHT){
      buffer.line(mWidth - DEFAULTBORDERPENWIDTH, 0,
                  mWidth - DEFAULTBORDERPENWIDTH, mHeight);
    }
    if (mBorderKind & BORDERBOTTOM){
      buffer.line(mWidth - DEFAULTBORDERPENWIDTH,
                  mHeight - DEFAULTBORDERPENWIDTH,
                  -DEFAULTBORDERPENWIDTH, mHeight - DEFAULTBORDERPENWIDTH);
    }
    if (mBorderKind & BORDERLEFT){
      buffer.line(0, mHeight - DEFAULTBORDERPENWIDTH,
                  0, -DEFAULTBORDERPENWIDTH);
    }
  }

  PaintTitle(buffer);
  PaintComment(buffer);
  PaintValue(buffer);
}

void InfoBox::PaintFast(void) {
  on_paint(get_canvas());
}

void
InfoBox::PaintInto(Canvas &dest, int xoff, int yoff, int width, int height)
{
  dest.stretch(xoff, yoff, width, height,
               buffer, 0, 0, mWidth, mHeight);
}

Canvas &InfoBox::GetCanvas(void) {
  return buffer;
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

void InfoBox::on_mouse_down(unsigned x, unsigned y)
{
  /* synthetic double click detection with no proximity , good for
     infoboxes */
  static PeriodClock double_click;

  if (!double_click.check_always_update(DOUBLECLICKINTERVAL)) {
#ifdef DEBUG_DBLCLK
    DoStatusMessage(_T("synth DBLCLK InfoBox!")); // VENTA3
#endif
    InputEvents::ShowMenu();
    return;
  }

#ifdef DEBUG_DBLCLK
  DoStatusMessage(_T("BDOWN InfoBox")); // VENTA3
#endif
  GetParent().send_command(GetHandle());
}

void InfoBox::on_mouse_double(unsigned x, unsigned y)
{
#ifndef GNAV
  // JMW capture double click, so infoboxes double clicked also bring up menu
  // VENTA3: apparently this is working only on PC ! Disable it to let PC work
  // with same timeout of PDA and PNA versions with synthetic DBLCLK
#ifdef DEBUG_DBLCLK
  DoStatusMessage(_T("DBLCLK InfoBox")); // VENTA3
#endif
  InputEvents::ShowMenu();
#endif
}

void InfoBox::on_paint(Canvas &canvas)
{
  canvas.copy(buffer);
  PaintSelector(canvas);
}

void InitInfoBoxModule(void){

  static bool InitDone = false;

  if (InitDone)
    return;

  InitDone = true;
}
