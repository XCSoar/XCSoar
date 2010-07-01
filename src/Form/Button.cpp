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

#include "Form/Button.hpp"
#include "Form/Control.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Interface.hpp"

WndButton::WndButton(ContainerWindow &parent,
    const TCHAR *Caption, int X, int Y, int Width, int Height,
                     const WindowStyle style,
                     Color background_color,
    ClickNotifyCallback_t Function) :
  text_color(Color::BLACK),
  background_brush(background_color),
  mDown(false),
  mLastDrawTextHeight(-1),
  mOnClickNotify(Function)
{
  // copy the buttons initial caption to the mCaption field
  _tcscpy(mCaption, Caption);

#if defined(WIN32) && !defined(NDEBUG)
  ::SetWindowText(hWnd, Caption);
#endif

  set(parent, X, Y, Width, Height, style);
}

void
WndButton::on_click()
{
  // Call the OnClick function
  if (mOnClickNotify != NULL)
    mOnClickNotify(*this);
}

bool
WndButton::on_setfocus()
{
  PaintWindow::on_setfocus();
  invalidate();
  return true;
}

bool
WndButton::on_killfocus()
{
  PaintWindow::on_killfocus();
  invalidate();
  return true;
}

bool
WndButton::on_mouse_up(int x, int y)
{
  // If button does not have capture -> call parent function
  if (!has_capture())
    return PaintWindow::on_mouse_up(x, y);

  release_capture();

  // If button hasn't been pressed, mouse is only released over it -> return
  if (!mDown)
    return true;

  // Button is not pressed anymore
  mDown = false;

  // Repainting needed
  invalidate();

  on_click();

  return true;
}

bool
WndButton::on_mouse_down(int x, int y)
{
  (void)x;
  (void)y;

  // Button is now pressed
  mDown = true;

  if (has_focus())
    // If button has focus -> repaint
    invalidate();
  else
    // If button not yet focused -> give focus to the button
    set_focus();

  set_capture();

  return true;
}

bool
WndButton::on_mouse_move(int x, int y, unsigned keys)
{
  // If button does not have capture -> call parent function
  if (!has_capture())
    return PaintWindow::on_mouse_move(x, y, keys);

  // If button is currently pressed and mouse cursor is moving on top of it
  bool in = in_client_rect(x, y);
  if (in != mDown) {
    mDown = in;
    invalidate();
  }

  return true;
}

bool
WndButton::on_key_check(unsigned key_code)
{
  switch (key_code) {
  case VK_RETURN:
    return true;

  default:
    return false;
  }
}

bool
WndButton::on_key_down(unsigned key_code)
{
  switch (key_code) {
#ifdef GNAV
  // JMW added this to make data entry easier
  case VK_F4:
#endif
  case VK_RETURN:
  case VK_SPACE:
    // "Press" the button via keys and repaint it
    if (!mDown) {
      mDown = true;
      invalidate();
    }
    return true;
  }

  // If key_down hasn't been handled yet -> call parent function
  return PaintWindow::on_key_down(key_code);
}

bool
WndButton::on_key_up(unsigned key_code)
{
  switch (key_code) {
#ifdef GNAV
  // JMW added this to make data entry easier
  case VK_F4:
#endif
  case VK_RETURN:
  case VK_SPACE:
    if (!mDown)
      return true;

    // Button is not pressed anymore
    mDown = false;
    invalidate();

    // Return if button was not pressed long enough
    if (!XCSoarInterface::Debounce())
      return true; // prevent false trigger

    on_click();

    return true;
  }

  // If key_down hasn't been handled yet -> call parent function
  return PaintWindow::on_key_up(key_code);
}

void
WndButton::on_paint(Canvas &canvas)
{
  /* background and selector */
  RECT rc = get_client_rect();
  canvas.fill_rectangle(rc, background_brush);

  if (has_focus())
    WindowControl::PaintSelector(canvas, get_client_rect());

  // Get button RECT and shrink it to make room for the selector/focus
  InflateRect(&rc, -2, -2); // todo border width

  // JMW todo: add icons?

  // Draw button to the background
  canvas.draw_button(rc, mDown);

  // If button has text on it
  if (mCaption == NULL || mCaption[0] == '\0')
    return;

  // Set drawing colors
  canvas.set_text_color(text_color);
  canvas.background_transparent();

  // Set drawing font
  canvas.select(MapWindowBoldFont);

  if (mLastDrawTextHeight < 0) {
    // Calculate the text height and save it for the future
    RECT rc_t = rc;
    canvas.formatted_text(&rc_t, mCaption,
        DT_CALCRECT | DT_EXPANDTABS | DT_CENTER | DT_NOCLIP | DT_WORDBREAK);

    mLastDrawTextHeight = rc_t.bottom - rc_t.top;
  }

  // If button is pressed, offset the text for 3D effect
  if (mDown)
    OffsetRect(&rc, Layout::FastScale(1), Layout::FastScale(1));

  // Vertical middle alignment
  rc.top += (canvas.get_height() - 4 - mLastDrawTextHeight) / 2;

  // Draw the button caption
  canvas.formatted_text(&rc, mCaption,
      DT_EXPANDTABS | DT_CENTER | DT_NOCLIP | DT_WORDBREAK);
}

void
WndButton::SetCaption(const TCHAR *Value)
{
  _tcscpy(mCaption, Value);
  mLastDrawTextHeight = -1;
  invalidate();
}

