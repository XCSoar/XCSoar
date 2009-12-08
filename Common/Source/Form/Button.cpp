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
#include "Screen/Animation.hpp"
#include "Interface.hpp"

WndButton::WndButton(WindowControl *Parent,
                     const TCHAR *Name, const TCHAR *Caption,
                     int X, int Y, int Width, int Height,
                     void (*Function)(WindowControl *Sender))
      :WindowControl(Parent, NULL /*Parent->GetHandle()*/, Name, X, Y, Width, Height)
{
  SetCanFocus(true);

  mOnClickNotify = Function;
  mDown = false;
  mDefault = false;

  SetForeColor(GetOwner()->GetForeColor());
  SetBackColor(GetOwner()->GetBackColor());

  _tcscpy(mCaption, Caption);

  mLastDrawTextHeight = -1;

}

bool
WndButton::on_mouse_up(int x, int y)
{
  if (has_capture()) {
    release_capture();

    if (!mDown)
      return true;

    mDown = false;
    invalidate();

    if (mOnClickNotify != NULL) {
      RECT mRc = get_screen_position();
      SetSourceRectangle(mRc);
      (mOnClickNotify)(this);
    }

    return true;
  } else
    return WindowControl::on_mouse_up(x, y);
}


bool
WndButton::on_key_down(unsigned key_code)
{
#ifdef VENTA_DEBUG_EVENT
  TCHAR ventabuffer[80];
  wsprintf(ventabuffer,TEXT("ONKEYDOWN key_code=%d"), key_code); // VENTA-
  DoStatusMessage(ventabuffer);
#endif
  switch (key_code){
#ifdef GNAV
    // JMW added this to make data entry easier
    case VK_F4:
#endif
    case VK_RETURN:
    case VK_SPACE:
      if (!mDown){
        mDown = true;
        invalidate();
      }
      return true;
  }

  return WindowControl::on_key_down(key_code);
}

bool
WndButton::on_key_up(unsigned key_code)
{
  switch (key_code){
#ifdef GNAV
    // JMW added this to make data entry easier
    case VK_F4:
#endif
    case VK_RETURN:
    case VK_SPACE:
      if (!XCSoarInterface::Debounce())
        return 1; // prevent false trigger
      if (mDown){
        mDown = false;
        invalidate();

        if (mOnClickNotify != NULL) {
          RECT mRc = get_screen_position();
          SetSourceRectangle(mRc);
          (mOnClickNotify)(this);
        }
      }
      return true;
  }

  return WindowControl::on_key_up(key_code);
}

bool
WndButton::on_mouse_down(int x, int y)
{
  (void)x; (void)y;
  mDown = true;
  if (!GetFocused())
    set_focus();
  else
    invalidate();

  set_capture();
  return true;
}

bool
WndButton::on_mouse_move(int x, int y, unsigned keys)
{
  if (has_capture()) {
    bool in = in_client_rect(x, y);
    if (in != mDown) {
      mDown = in;
      invalidate();
    }

    return true;
  } else
    return WindowControl::on_mouse_move(x, y, keys);
}

bool
WndButton::on_mouse_double(int x, int y)
{
  (void)x; (void)y;
  mDown = true;
  invalidate();
  set_capture();
  return true;
}


void
WndButton::on_paint(Canvas &canvas)
{
  WindowControl::on_paint(canvas);

  RECT rc = get_client_rect();
  InflateRect(&rc, -2, -2); // todo border width

  // JMW todo: add icons?

  canvas.draw_button(rc, mDown);

  if (mCaption != NULL && mCaption[0] != '\0'){
    canvas.set_text_color(GetForeColor());
    canvas.set_background_color(GetBackColor());
    canvas.background_transparent();

    canvas.select(*GetFont());

    rc = get_client_rect();
    InflateRect(&rc, -2, -2); // todo border width

    if (mDown)
      OffsetRect(&rc, 2, 2);

    if (mLastDrawTextHeight < 0){
      canvas.formatted_text(&rc, mCaption,
          DT_CALCRECT
        | DT_EXPANDTABS
        | DT_CENTER
        | DT_NOCLIP
        | DT_WORDBREAK // mCaptionStyle // | DT_CALCRECT
      );

      mLastDrawTextHeight = rc.bottom - rc.top;
      // DoTo optimize
      rc = get_client_rect();
      InflateRect(&rc, -2, -2); // todo border width
      if (mDown)
        OffsetRect(&rc, 2, 2);

    }

    rc.top += (canvas.get_height() - 4 - mLastDrawTextHeight) / 2;

    canvas.formatted_text(&rc, mCaption,
        DT_EXPANDTABS
      | DT_CENTER
      | DT_NOCLIP
      | DT_WORDBREAK // mCaptionStyle // | DT_CALCRECT
    );

//    mLastDrawTextHeight = rc.bottom - rc.top;

  }

//  UINT lastAlign = SetTextAlign(hDC, TA_CENTER /*| VTA_CENTER*/);
//  ExtTextOut(hDC, GetWidth()/2, GetHeight()/2,
//    /*ETO_OPAQUE | */ETO_CLIPPED, &r, mCaption, _tcslen(mCaption), NULL);
//  if (lastAlign != GDI_ERROR){
//    SetTextAlign(hDC, lastAlign);
//  }


// 20060518:sgi old version
//  ExtTextOut(hDC, org.x, org.y,
//    /*ETO_OPAQUE | */ETO_CLIPPED, &r, mCaption, _tcslen(mCaption), NULL);
}
