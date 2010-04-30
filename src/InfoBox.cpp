/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "InfoBox.hpp"
#include "InfoBoxManager.hpp"
#include "Protection.hpp"
#include "Dialogs.h"
#include "InputEvents.h"
#include "Compatibility/string.h"
#include "PeriodClock.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Screen/Layout.hpp"
#include "SettingsUser.hpp"
#include "Appearance.hpp"
#include "Defines.h"
#include "UtilsSystem.hpp"
#include "Asset.hpp"

#include <algorithm>

using std::max;

static const Color bkColorSel(150, 0x0, 0x0);
static const Color bdColor(80, 80, 80);
static Pen hPenDefaultBorder;
static Pen hPenSelector;
static int Count = 0;

#define SELECTORWIDTH IBLSCALE(5)

InfoBox::InfoBox(ContainerWindow &parent, int X, int Y, int Width, int Height)
  :focus_timer(0)
{
  mX = X;
  mY = Y;
  mWidth = Width;
  mHeight = Height;

  mTitleChanged = true;
  mSmallerFont = false;

  color = 0;
  colorTop = 0;
  colorBottom = 0;

  set(parent, mX, mY, mWidth, mHeight);

  Color fgColor, bkColor;

  if (Appearance.InverseInfoBox) {
    fgColor = Color(0xff, 0xff, 0xff);
    bkColor = Color(0x00, 0x00, 0x00);
  } else {
    bkColor = Color(0xff, 0xff, 0xff);
    fgColor = Color(0x00, 0x00, 0x00);
  }

  if (Count == 0) {
    hPenDefaultBorder.set(BORDER_WIDTH, bdColor);
    hPenSelector.set(IBLSCALE(1) + 2, fgColor);
  }

  mhPenBorder = hPenDefaultBorder;
  mhPenSelector = hPenSelector;

  if (Appearance.InfoBoxBorder == apIbTab) {
    mBorderKind = BORDERTAB;
  } else {
    mBorderKind = BORDERRIGHT | BORDERBOTTOM;
  }

  mphFontTitle = &TitleWindowFont;
  mphFontValue = &InfoWindowFont;
  mphFontComment = &TitleWindowFont;
  valueFont = &TitleSmallWindowFont;

  mColorTitle = fgColor;
  mColorValue = fgColor;
  mColorComment = fgColor;

  mColorTitleBk = bkColor;
  mColorValueBk = bkColor;
  mColorCommentBk = bkColor;

  mValueUnit = unUndef;

  _tcscpy(mTitle, TEXT(""));
  _tcscpy(mValue, TEXT(""));
  _tcscpy(mComment, TEXT(""));

  mHasFocus = false;

  Count++;
}

InfoBox::~InfoBox(void){
  Count--;

  if (Count == 0) {
    hPenDefaultBorder.reset();
    hPenSelector.reset();
  }
}

void
InfoBox::SetValueUnit(Units_t Value)
{
  mValueUnit = Value;
}

int
InfoBox::GetBorderKind(void)
{
  return mBorderKind;
}

int
InfoBox::SetBorderKind(int Value)
{
  int res = mBorderKind;

  if (mBorderKind != Value) {
    mBorderKind = Value;

    if (Appearance.InfoBoxBorder == apIbTab) {
      mBorderKind = BORDERTAB;
    } else {
      mBorderKind = Value;
    }
    //JMW    Paint();
  }

  return res;
}

void
InfoBox::SetTitle(const TCHAR *Value)
{
  TCHAR sTmp[TITLESIZE + 1];

  _tcsncpy(sTmp, Value, TITLESIZE);
  sTmp[TITLESIZE] = '\0';

  if (Appearance.InfoTitelCapital)
    _tcsupr(sTmp);

  if (_tcscmp(mTitle, sTmp) != 0) {
    _tcscpy(mTitle, sTmp);
    mTitleChanged = true;
    //JMW    PaintTitle();
    //JMW    PaintSelector();
  }
}

void
InfoBox::SetValue(const TCHAR *Value)
{
  if (_tcscmp(mValue, Value) != 0) {
    _tcsncpy(mValue, Value, VALUESIZE);
    mValue[VALUESIZE] = '\0';
    //JMW    PaintValue();
  }
}

void
InfoBox::SetColor(int value)
{
  if (Appearance.InfoBoxColors)
    color = value;
  else
    color = 0;
}

void
InfoBox::SetColorBottom(int value)
{
  if (Appearance.InfoBoxColors)
    colorBottom = value;
  else
    colorBottom = 0;
}

void
InfoBox::SetColorTop(int value)
{
  if (Appearance.InfoBoxColors)
    colorTop = value;
  else
    colorTop = 0;
}

void
InfoBox::SetComment(const TCHAR *Value)
{
  if (_tcscmp(mComment, Value) != 0) {
    _tcsncpy(mComment, Value, COMMENTSIZE);
    mComment[COMMENTSIZE] = '\0';
    //JMW    PaintComment();
    //JMW    PaintSelector();
  }
}

void
InfoBox::SetSmallerFont(bool smallerFont)
{
  this->mSmallerFont = smallerFont;
}

void
InfoBox::PaintTitle(Canvas &canvas)
{
  if (!mTitleChanged)
    return;

  SIZE tsize;
  int x, y;
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
    canvas.set_text_color(Appearance.InverseInfoBox ?
                          MapGfx.inv_redColor : Color::RED);
    break;
  case 2:
    canvas.set_text_color(Appearance.InverseInfoBox ?
                          MapGfx.inv_blueColor : Color::BLUE);
    break;
    // VENTA3 added colors
  case 3:
    canvas.set_text_color(Appearance.InverseInfoBox ?
                          MapGfx.inv_greenColor : Color::GREEN);
    break;
  case 4:
    canvas.set_text_color(Appearance.InverseInfoBox ?
                          MapGfx.inv_yellowColor : Color::YELLOW);
    break;
  case 5:
    canvas.set_text_color(Appearance.InverseInfoBox ?
                          MapGfx.inv_magentaColor : Color::MAGENTA);
    break;
  }

  canvas.select(*mphFontTitle);

  tsize = canvas.text_size(mTitle);

  halftextwidth = (mWidth - tsize.cx) >> 1;

  x = max(1, (int)recTitle.left + halftextwidth);

  y = recTitle.top + 1 + mphFontTitle->get_capital_height()
    - mphFontTitle->get_ascent_height();

  canvas.text_opaque(x, y, &recTitle, mTitle);

  if ((mBorderKind & BORDERTAB) && (halftextwidth > IBLSCALE(3))) {
    int ytop = recTitle.top + mphFontTitle->get_capital_height() / 2;
    int ytopedge = ytop + IBLSCALE(2);
    int ybottom = recTitle.top + IBLSCALE(6) +
      mphFontTitle->get_capital_height();

    canvas.select(mhPenBorder);

    POINT tab[8];
    tab[0].x = tab[1].x = recTitle.left + IBLSCALE(1);
    tab[0].y = tab[7].y = ybottom;
    tab[2].x = recTitle.left + IBLSCALE(3);
    tab[2].y = tab[5].y = tab[3].y = tab[4].y = ytop;
    tab[1].y = tab[6].y = ytopedge;
    tab[5].x = recTitle.right - IBLSCALE(4);
    tab[6].x = tab[7].x = recTitle.right - IBLSCALE(2);
    tab[3].x = recTitle.left + halftextwidth - IBLSCALE(1);
    tab[4].x = recTitle.right - halftextwidth + IBLSCALE(1);

    canvas.polyline(tab, 4);
    canvas.polyline(tab + 4, 4);
  }

  mTitleChanged = false;
}

void
InfoBox::PaintValue(Canvas &canvas)
{
  SIZE tsize;
  int x, y;

  canvas.set_background_color(mColorValueBk);

  switch (color) {
  case -1:
    canvas.set_text_color(bdColor);
    break;
  case 0:
    canvas.set_text_color(mColorValue);
    break;
  case 1:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_redColor);
     else
      canvas.set_text_color(Color::RED);

    break;
  case 2:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_blueColor);
    else
      canvas.set_text_color(Color::BLUE);

    break;
// VENTA3 more colors
  case 3:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_greenColor);
    else
      canvas.set_text_color(Color::GREEN);

    break;
  case 4:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_yellowColor);
    else
      canvas.set_text_color(Color::YELLOW);

    break;
  case 5:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_magentaColor);
    else
      canvas.set_text_color(Color::MAGENTA);

    break;
  }

  canvas.select(mSmallerFont ? *valueFont : *mphFontValue);

  tsize = canvas.text_size(mValue);

  SIZE unit_size;
  const UnitSymbol *unit_symbol = GetUnitSymbol(mValueUnit);
  if (unit_symbol != NULL) {
    unit_size = unit_symbol->get_size();
  } else {
    unit_size.cx = 0;
    unit_size.cy = 0;
  }

  x = max(1, (int)recValue.left +
          (mWidth - (int)tsize.cx - Layout::FastScale(unit_size.cx)) / 2);

  y = recValue.top + 1 - mphFontValue->get_ascent_height() +
    (recValue.bottom - recValue.top + mphFontValue->get_capital_height()) / 2;

  canvas.text_opaque(x, y, &recValue, mValue);

  if (unit_symbol != NULL && color >= 0) {
    POINT origin = unit_symbol->get_origin(Appearance.InverseInfoBox
                                           ? UnitSymbol::INVERSE
                                           : UnitSymbol::NORMAL);

    BitmapCanvas temp(canvas, *unit_symbol);

    canvas.scale_copy(x + tsize.cx,
                      y + mphFontValue->get_ascent_height()
                      - Layout::FastScale(unit_size.cy),
                      temp,
                      origin.x, origin.y,
                      unit_size.cx, unit_size.cy);
  }
}

void
InfoBox::PaintComment(Canvas &canvas)
{
  SIZE tsize;
  int x, y;

  switch (colorBottom) {
  case -1:
    canvas.set_text_color(bdColor);
    break;
  case 0:
    canvas.set_text_color(mColorValue);
    break;
  case 1:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_redColor);
    else
      canvas.set_text_color(Color::RED);

    break;
  case 2:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_blueColor);
    else
      canvas.set_text_color(Color::BLUE);

    break;
    // VENTA3 more colors
  case 3:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_greenColor);
    else
      canvas.set_text_color(Color::GREEN);

    break;
  case 4:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_yellowColor);
    else
      canvas.set_text_color(Color::YELLOW);

    break;
  case 5:
    if (Appearance.InverseInfoBox)
      canvas.set_text_color(MapGfx.inv_magentaColor);
    else
      canvas.set_text_color(Color::MAGENTA);

    break;
  }

  canvas.set_background_color(mColorCommentBk);

  // SetTextColor(mHdcBuf, mColorComment);

  canvas.select(*mphFontComment);

  tsize = canvas.text_size(mComment);

  x = max(1, (int)recComment.left + (mWidth - (int)tsize.cx) / 2);
  y = recComment.top + 1 + mphFontComment->get_capital_height()
    - mphFontComment->get_ascent_height();

  canvas.text_opaque(x, y, &recComment, mComment);
}

void
InfoBox::PaintSelector(Canvas &canvas)
{
  if (mHasFocus) {
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

void
InfoBox::Paint()
{
  // safety
  if (!globalRunningEvent.test())
    return;

  static bool InitDone = false;

  if (!InitDone) {
    InitializeDrawHelpers();
    InitDone = false;
  }

  Canvas &buffer = get_canvas();
  buffer.background_opaque();

  PaintTitle(buffer);
  PaintComment(buffer);
  PaintValue(buffer);

  if (mBorderKind != 0) {
    buffer.select(mhPenBorder);

    if (mBorderKind & BORDERTOP) {
      buffer.line(0, 0, mWidth - 1, 0);
    }

    if (mBorderKind & BORDERRIGHT) {
      buffer.line(mWidth - 1, 0, mWidth - 1, mHeight);
    }

    if (mBorderKind & BORDERBOTTOM) {
      buffer.line(0, mHeight - 1, mWidth - 1, mHeight - 1);
    }

    if (mBorderKind & BORDERLEFT) {
      buffer.line(0, 0, 0, mHeight - 1);
    }
  }
}

void
InfoBox::PaintInto(Canvas &dest, int xoff, int yoff, int width, int height)
{
  dest.stretch(xoff, yoff, width, height, get_canvas(), 0, 0, mWidth, mHeight);
}

void
InfoBox::InitializeDrawHelpers(void)
{
  RECT rc = get_client_rect();

  if (mBorderKind & BORDERLEFT)
    rc.left += BORDER_WIDTH;

  if (mBorderKind & BORDERRIGHT)
    rc.right -= BORDER_WIDTH;

  if (mBorderKind & BORDERTOP)
    rc.top += BORDER_WIDTH;

  if (mBorderKind & BORDERBOTTOM)
    rc.bottom -= BORDER_WIDTH;

  recTitle = rc;
  recTitle.bottom = rc.top + mphFontTitle->get_capital_height() + 2;

  recComment = rc;
  recComment.top = recComment.bottom
    - (mphFontTitle->get_capital_height() + 2);

  recValue = rc;
  recValue.top = recTitle.bottom;
  recValue.bottom = recComment.top;
}

bool
InfoBox::on_key_down(unsigned key_code)
{
  // Get the input event_id of the event
  unsigned event_id = InputEvents::key_to_event(InputEvents::MODE_INFOBOX,
                                                key_code);
  if (event_id > 0) {
    // If input event exists -> process it
    InputEvents::processGo(event_id);

    // restart focus timer if not idle
    if (focus_timer != 0)
      kill_timer(focus_timer);

    focus_timer = set_timer(100, FOCUSTIMEOUTMAX * 500);
    return true;
  }

  return BufferWindow::on_key_down(key_code);
}

bool
InfoBox::on_mouse_down(int x, int y)
{
  // synthetic double click detection with no proximity , good for infoboxes
  static PeriodClock double_click;

  // if double clicked -> show menu
  if (!double_click.check_always_update(DOUBLECLICKINTERVAL)) {
    InputEvents::ShowMenu();
    return true;
  }

  // if single clicked -> focus the InfoBox
  set_focus();
  return true;
}

bool
InfoBox::on_mouse_double(int x, int y)
{
  if (!is_altair()) {
    // JMW capture double click, so infoboxes double clicked also bring up menu
    // VENTA3: apparently this is working only on PC ! Disable it to let PC work
    // with same timeout of PDA and PNA versions with synthetic DBLCLK
    InputEvents::ShowMenu();
  }

  return true;
}

void
InfoBox::on_paint(Canvas &canvas)
{
  // Call the parent function
  BufferWindow::on_paint(canvas);
  // Paint the selector
  PaintSelector(canvas);
}

bool
InfoBox::on_setfocus()
{
  // Call the parent function
  BufferWindow::on_setfocus();

  // Save the focus state
  mHasFocus = true;

  // Start the focus-auto-return timer
  // to automatically return focus back to MapWindow if idle
  focus_timer = set_timer(100, FOCUSTIMEOUTMAX * 500);

  // Redraw fast to paint the selector
  invalidate();

  return true;
}

bool
InfoBox::on_killfocus()
{
  // Call the parent function
  BufferWindow::on_killfocus();

  // Save the unfocused state
  mHasFocus = false;

  // Destroy the time if it exists
  if (focus_timer != 0) {
    kill_timer(focus_timer);
    focus_timer = 0;
  }

  // Redraw fast to remove the selector
  invalidate();

  return true;
}

bool
InfoBox::on_timer(timer_t id)
{
  if (id != focus_timer)
    return BufferWindow::on_timer(id);

  kill_timer(focus_timer);
  focus_timer = 0;

  CommonInterface::main_window.map.set_focus();

  return true;
}
