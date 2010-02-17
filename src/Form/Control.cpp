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

#include "Form/Control.hpp"
#include "Form/Container.hpp"
#include "Form/Internal.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs.h"
#include "PeriodClock.hpp"

#include <stdlib.h>

bool WindowControl::initialized;
Brush WindowControl::hBrushDefaultBk;

// returns true if it is a long press,
// otherwise returns false
bool
KeyTimer(bool isdown, unsigned thekey)
{
  static PeriodClock fps_time_down;
  static DWORD savedKey = 0;

  if (thekey == savedKey && fps_time_down.check_update(2000)) {
    savedKey = 0;
    return true;
  }

  if (!isdown) {
    // key is released
  } else {
    // key is lowered
    if (thekey != savedKey) {
      fps_time_down.update();
      savedKey = thekey;
    }
  }
  return false;
}

WindowControl::WindowControl(ContainerControl *Owner, ContainerWindow *Parent,
                             int X, int Y, int Width, int Height,
                             const WindowStyle style) :
    mOwner(Owner),
    mColorBack(Color::WHITE),
    mColorFore(Color::BLACK),
    mhFont(&MapWindowFont),
    mHelpText(NULL),
    mOnHelpCallback(NULL),
    mHasFocus(false),
    mPaintSelector(true)
{
  // Clear the caption
  mCaption[0] = '\0';

  if ((Parent == NULL) && (mOwner != NULL))
    Parent = (ContainerWindow *)&mOwner->GetClientAreaWindow();

  // If not done already -> initialize default brushes and pens
  if (!initialized) {
    hBrushDefaultBk.set(mColorBack);
    initialized = true;
  }

  set(*Parent, X, Y, Width, Height, style);

  // Add the Control as a client of its parent
  if (mOwner != NULL) {
    SetFont(mOwner->GetFont());

    mOwner->AddClient(this);
  }
}

WindowControl::~WindowControl(void)
{
  if (mHelpText) {
    free(mHelpText);
    mHelpText = NULL;
  }
}

void
WindowControl::SetHelpText(const TCHAR *Value)
{
  if (mHelpText) {
    free(mHelpText);
    mHelpText = NULL;
  }

  if (Value == NULL)
    return;

  int len = _tcslen(Value);

  if (len == 0)
    return;

  mHelpText = (TCHAR*)malloc((len + 1) * sizeof(TCHAR));
  if (mHelpText != NULL)
    _tcscpy(mHelpText, Value);
}

void
WindowControl::SetCaption(const TCHAR *Value)
{
  if (Value == NULL)
    Value = TEXT("");

  if (_tcscmp(mCaption, Value) != 0) {
    _tcscpy(mCaption, Value);
    invalidate();
  }
}

bool
WindowControl::SetFocused(bool Value)
{
  bool res = mHasFocus;

  if (mHasFocus != Value) {
    mHasFocus = Value;

    // todo, only paint the selector edges
    invalidate();
  }

  return res;
}

const Font *
WindowControl::SetFont(const Font &Value)
{
  const Font *res = mhFont;

  if (mhFont != &Value) {
    // todo
    mhFont = &Value;
    invalidate();
  }
  return res;
}

Color
WindowControl::SetForeColor(Color Value)
{
  Color res = mColorFore;

  if (mColorFore != Value) {
    mColorFore = Value;
    invalidate();
  }

  return res;
}

Color
WindowControl::SetBackColor(Color Value)
{
  Color res = mColorBack;

  if (mColorBack != Value) {
    mColorBack = Value;
    mhBrushBk.set(mColorBack);
    invalidate();
  }

  return res;
}

void
WindowControl::PaintSelector(Canvas &canvas, const RECT rc)
{
  const Pen pen(DEFAULTBORDERPENWIDTH + 2, Color::BLACK);
  canvas.select(pen);

  canvas.two_lines(rc.right - SELECTORWIDTH - 1, rc.top,
                   rc.right - 1, rc.top,
                   rc.right - 1, rc.top + SELECTORWIDTH + 1);

  canvas.two_lines(rc.right - 1, rc.bottom - SELECTORWIDTH - 2,
                   rc.right - 1, rc.bottom - 1,
                   rc.right - SELECTORWIDTH - 1, rc.bottom - 1);

  canvas.two_lines(SELECTORWIDTH + 1, rc.bottom - 1,
                   rc.left, rc.bottom - 1,
                   rc.left, rc.bottom - SELECTORWIDTH - 2);

  canvas.two_lines(rc.left, rc.top + SELECTORWIDTH + 1,
                   rc.left, rc.top,
                   rc.left + SELECTORWIDTH + 1, rc.top);
}

void
WindowControl::PaintSelector(Canvas &canvas)
{
  if (mPaintSelector && mHasFocus)
    PaintSelector(canvas, get_client_rect());
}

int
WindowControl::OnHelp()
{
#ifdef ALTAIRSYNC
  return 0; // undefined. return 1 if defined
#else
  if (mHelpText) {
    dlgHelpShowModal(*(SingleWindow *)get_root_owner(), mCaption, mHelpText);
    return 1;
  }

  if (mOnHelpCallback) {
    (mOnHelpCallback)(this);
    return 1;
  }

  return 0;
#endif
}

bool
WindowControl::on_key_down(unsigned key_code)
{
  // JMW: HELP
  KeyTimer(true, key_code);

  return ContainerWindow::on_key_down(key_code) || on_unhandled_key(key_code);
}

bool
WindowControl::on_key_up(unsigned key_code)
{
  // JMW: detect long enter release
  // VENTA4: PNAs don't have Enter, so it should be better to find an alternate solution
  // activate tool tips if hit return for long time
  if (KeyTimer(false, key_code) && key_code == VK_RETURN && OnHelp())
    return true;

  return ContainerWindow::on_key_up(key_code);
}

void
WindowControl::on_paint(Canvas &canvas)
{
  const RECT rc = get_client_rect();

  // JMW added highlighting, useful for lists
  if (mPaintSelector && mHasFocus) {
    Color ff = GetBackColor().highlight();
    Brush brush(ff);
    canvas.fill_rectangle(rc, brush);

#ifdef WINDOWSPC
    // JMW make it look nice on wine
    canvas.set_background_color(ff);
#endif
  } else
    canvas.fill_rectangle(rc, GetBackBrush());

  PaintSelector(canvas);

  ContainerWindow::on_paint(canvas);
}

bool
WindowControl::on_setfocus()
{
  ContainerWindow::on_setfocus();
  SetFocused(true);
  return true;
}

bool
WindowControl::on_killfocus()
{
  ContainerWindow::on_killfocus();
  SetFocused(false);
  return true;
}

bool
WindowControl::on_unhandled_key(unsigned key_code)
{
  if (mOwner != NULL && mOwner->on_unhandled_key(key_code))
    return true;

  return false;
}
