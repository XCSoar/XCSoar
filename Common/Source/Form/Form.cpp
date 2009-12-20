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

#include "Form/Form.hpp"
#include "Form/Internal.hpp"
#include "PeriodClock.hpp"
#include "Asset.hpp"
#include "Interface.hpp"
#include "MapWindow.h"
#include "Screen/Animation.hpp"

PeriodClock WndForm::timeAnyOpenClose;

WndForm::WndForm(ContainerWindow *Parent,
                 const TCHAR *Name, const TCHAR *Caption,
                 int X, int Y, int Width, int Height):
  WindowControl(NULL, Parent, Name, X, Y, Width, Height, false),
  mModalResult(0),
  mColorTitle(Color::YELLOW),
  mhTitleFont(GetFont()),
  mClientWindow(NULL),
  mOnTimerNotify(NULL), mOnKeyDownNotify(NULL), mOnUserMsgNotify(NULL)
{
  mClientWindow = new WindowControl(this, this,
                                    TEXT(""), 20, 20, Width, Height);
  mClientWindow->SetBackColor(GetBackColor());

  mClientRect.top=0;
  mClientRect.left=0;
  mClientRect.bottom=Width;
  mClientRect.right=Height;

  cbTimerID = set_timer(1001, 500);

  if (Caption != NULL)
    _tcscpy(mCaption, Caption);

}

ContainerWindow &
WndForm::GetClientAreaWindow(void)
{

  if (mClientWindow != NULL)
    return *mClientWindow;
  else
    return *this;
}


void WndForm::AddClient(WindowControl *Client){      // add client window
  if (mClientWindow != NULL){
    mClientWindow->AddClient(Client); // add it to the clientarea window
  } else
    WindowControl::AddClient(Client);
}

bool
WndForm::on_timer(timer_t id)
{
  if (id == cbTimerID) {
    if (mOnTimerNotify)
      mOnTimerNotify(this);
    return true;
  } else
    return WindowControl::on_timer(id);
}

bool
WndForm::on_user(unsigned id)
{
  if (mOnUserMsgNotify != NULL && mOnUserMsgNotify(this, id))
    return true;

  return WindowControl::on_user(id);
}

const Font *
WndForm::SetTitleFont(const Font &font)
{
  const Font *res = mhTitleFont;

  if (mhTitleFont != &font){
    // todo
    mhTitleFont = &font;

    invalidate();
  }

  return res;

}

int WndForm::ShowModal(void){
  return ShowModal(false);
}

#ifndef ENABLE_SDL

static bool
is_user_input(UINT message)
{
  return message == WM_KEYDOWN || message == WM_KEYUP ||
    message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ||
    message == WM_LBUTTONDBLCLK;
}

static bool
is_allowed_map_message(UINT message)
{
  return message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ||
    message == WM_MOUSEMOVE;
}

static bool
is_allowed_map(HWND hWnd, UINT message, bool enable_map)
{
  return !is_altair() && enable_map && MapWindow::identify(hWnd) &&
    is_allowed_map_message(message);
}

#endif /* !ENABLE_SDL */

int WndForm::ShowModal(bool bEnableMap) {
  assert_none_locked();

#define OPENCLOSESUPPRESSTIME 500
#ifndef ENABLE_SDL
  MSG msg;
  HWND oldFocusHwnd;
#endif /* !ENABLE_SDL */

  PeriodClock enter_clock;
  if (is_embedded() && !is_altair())
    enter_clock.update();

  RECT mRc = get_screen_position();
  DrawWireRects(XCSoarInterface::EnableAnimation,&mRc, 5);

  show_on_top();

  mModalResult = 0;

#ifndef ENABLE_SDL
  oldFocusHwnd = ::GetFocus();
#endif /* !ENABLE_SDL */
  set_focus();

  FocusNext(NULL);

#ifndef ENABLE_SDL
  bool hastimed = false;
#endif /* !ENABLE_SDL */
  WndForm::timeAnyOpenClose.update(); // when current dlg opens or child closes

#ifdef ENABLE_SDL

  update();

  SDL_Event event;
  while (SDL_WaitEvent(&event)) {
    if (event.type == SDL_QUIT)
      break;

    if (event.type >= SDL_USEREVENT && event.type <= SDL_NUMEVENTS-1 &&
        event.user.data1 != NULL) {
      Window *window = (Window *)event.user.data1;
      window->on_user(event.type - SDL_USEREVENT);
    } else
      parent->on_event(event);
  }

  return 0;

#else /* !ENABLE_SDL */
  while ((mModalResult == 0) && GetMessage(&msg, NULL, 0, 0)) {
//hack!

    // JMW update display timeout so we don't get blanking
    /*
    if (msg.message == WM_KEYDOWN) {
      if (!Debounce()) {
        continue;
      }
    }
    */

    if (msg.message == WM_KEYDOWN) {
      XCSoarInterface::InterfaceTimeoutReset();
    }

    if ((msg.message == WM_KEYDOWN) && ((msg.wParam & 0xffff) == VK_ESCAPE))
      mModalResult = mrCancel;

    if (is_user_input(msg.message)
        && !identify_descendant(msg.hwnd) // not current window or child
        && !is_allowed_map(msg.hwnd, msg.message, bEnableMap))
      continue;   // make it modal

    // hack to stop exiting immediately
    if (is_embedded() && !is_altair() && !hastimed &&
        is_user_input(msg.message)) {
      if (!enter_clock.check(1000))
        /* ignore user input in the first 1000ms */
        continue;
      else
        hastimed = true;
    }

    if (msg.message == WM_KEYDOWN && mOnKeyDownNotify != NULL &&
        mOnKeyDownNotify(this, msg.wParam))
      continue;

    TranslateMessage(&msg);
    if (msg.message != WM_LBUTTONUP ||
        // prevents child click from being repeat-handled by parent
        // if buttons overlap
        WndForm::timeAnyOpenClose.elapsed() > OPENCLOSESUPPRESSTIME) {
      assert_none_locked();

      DispatchMessage(&msg);

      assert_none_locked();
    }
  } // End Modal Loop
#endif /* !ENABLE_SDL */

  // static.  this is current open/close or child open/close
  WndForm::timeAnyOpenClose.update();

  //  SetSourceRectangle(mRc);
  //  DrawWireRects(&aniRect, 5);

  /*
  // reset to center?
  aniRect.top = (mRc.top+mRc.bottom)/2;;
  aniRect.left = (mRc.left+mRc.right)/2;
  aniRect.right = (mRc.left+mRc.right)/2;
  aniRect.bottom = (mRc.top+mRc.bottom)/2;
  SetSourceRectangle(aniRect);
  */

#ifndef ENABLE_SDL
  SetFocus(oldFocusHwnd);
#endif /* !ENABLE_SDL */

  return mModalResult;
}

void
WndForm::on_paint(Canvas &canvas)
{
  WindowControl::on_paint(canvas);

  RECT rcClient = get_client_rect();

  canvas.select(GetBorderPen());
  canvas.select(GetBackBrush());

  canvas.raised_edge(rcClient);

  canvas.set_text_color(GetForeColor());
  canvas.set_background_color(mColorTitle);
  canvas.background_transparent();

  canvas.select(*mhTitleFont);
  SIZE tsize = canvas.text_size(mCaption);

  // JMW todo add here icons?

  CopyRect(&mTitleRect, &rcClient);
  mTitleRect.bottom = mTitleRect.top + tsize.cy;

  rcClient.top += tsize.cy;

  if (mClientWindow && !EqualRect(&mClientRect, &rcClient)){
    mClientWindow->move(rcClient.left, rcClient.top);

    CopyRect(&mClientRect, &rcClient);

  }

  canvas.text_opaque(mTitleRect.left + 1, mTitleRect.top - 2,
                     &mTitleRect, mCaption);
}

void WndForm::SetCaption(const TCHAR *Value){
  if (Value == NULL)
    Value = TEXT("");

  if (_tcscmp(mCaption, Value) != 0){
    _tcscpy(mCaption, Value);
    invalidate(mTitleRect);
  }

}

Color WndForm::SetForeColor(Color Value)
{
  if (mClientWindow)
    mClientWindow->SetForeColor(Value);
  return WindowControl::SetForeColor(Value);
}

Color WndForm::SetBackColor(Color Value)
{
  if (mClientWindow)
  mClientWindow->SetBackColor(Value);
  return WindowControl::SetBackColor(Value);
}

const Font *WndForm::SetFont(const Font &Value){
  if (mClientWindow)
    mClientWindow->SetFont(Value);
  return WindowControl::SetFont(Value);
}

void
WndForm::SetKeyDownNotify(bool (*KeyDownNotify)(WindowControl *Sender,
                                                unsigned key_code))
{
  mOnKeyDownNotify = KeyDownNotify;
}

void
WndForm::SetTimerNotify(int (*OnTimerNotify)(WindowControl *Sender))
{
  mOnTimerNotify = OnTimerNotify;
}

void
WndForm::SetUserMsgNotify(bool (*OnUserMsgNotify)(WindowControl *Sender, unsigned id))
{
  mOnUserMsgNotify = OnUserMsgNotify;
}

// normal form stuff (nonmodal)

bool
WndForm::on_unhandled_key(unsigned key_code)
{
  if (mOnKeyDownNotify != NULL && mOnKeyDownNotify(this, key_code))
    return true;

  return WindowControl::on_unhandled_key(key_code);
}
