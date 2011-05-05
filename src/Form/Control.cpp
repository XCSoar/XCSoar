/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Form/Internal.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Dialogs/Dialogs.h"
#include "PeriodClock.hpp"
#include "Util/StringUtil.hpp"
#include "Language/Language.hpp"

#include <stdlib.h>

// returns true if it is a long press,
// otherwise returns false
bool
KeyTimer(bool isdown, unsigned thekey)
{
  static PeriodClock fps_time_down;
  static unsigned savedKey = 0;

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

WindowControl::WindowControl() :
    mColorBack(COLOR_WHITE),
    mColorFore(COLOR_BLACK),
    mhFont(&Fonts::Map),
    mHelpText(NULL),
    mOnHelpCallback(NULL)
{
  // Clear the caption
  mCaption[0] = '\0';
}

WindowControl::~WindowControl(void)
{
  free(mHelpText);
}

void
WindowControl::SetHelpText(const TCHAR *Value)
{
  free(mHelpText);
  mHelpText = Value != NULL ? _tcsdup(Value) : NULL;
}

void
WindowControl::SetCaption(const TCHAR *Value)
{
  if (Value == NULL)
    Value = _T("");

  if (_tcscmp(mCaption, Value) != 0) {
    _tcscpy(mCaption, Value);
    invalidate();
  }
}

void
WindowControl::SetFont(const Font &Value)
{
  if (mhFont != &Value) {
    // todo
    mhFont = &Value;
    invalidate();
  }
}

void
WindowControl::SetForeColor(Color Value)
{
  if (mColorFore != Value) {
    mColorFore = Value;
    invalidate();
  }
}

void
WindowControl::SetBackColor(Color Value)
{
  if (mColorBack != Value) {
    mColorBack = Value;
    invalidate();
  }
}

void
WindowControl::PaintSelector(Canvas &canvas, const PixelRect rc)
{
  const Pen pen(DEFAULTBORDERPENWIDTH + 2, COLOR_BLACK);
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

int
WindowControl::OnHelp()
{
#ifdef ALTAIRSYNC
  return 0; // undefined. return 1 if defined
#else
  if (mHelpText && !string_is_empty(mHelpText)) {
    dlgHelpShowModal(*(SingleWindow *)get_root_owner(), gettext(mCaption), gettext(mHelpText));
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

  return ContainerWindow::on_key_down(key_code);
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

bool
WindowControl::on_setfocus()
{
  ContainerWindow::on_setfocus();
  invalidate();
  return true;
}

bool
WindowControl::on_killfocus()
{
  ContainerWindow::on_killfocus();
  invalidate();
  return true;
}
