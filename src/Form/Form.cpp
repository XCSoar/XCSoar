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

#include "Form/Form.hpp"
#include "Form/Internal.hpp"
#include "PeriodClock.hpp"
#include "Asset.hpp"
#include "Interface.hpp"
#include "MapWindow.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Util/StringUtil.hpp"

#ifdef ANDROID
#include "Screen/Android/Event.hpp"
#include "Android/Main.hpp"
#elif defined(ENABLE_SDL)
#include "Screen/SDL/Event.hpp"
#else
#include "Screen/GDI/Event.hpp"
#endif

bool
WndForm::ClientAreaWindow::on_command(unsigned id, unsigned code)
{
  return (mCommandCallback != NULL && mCommandCallback(id))
    || ContainerWindow::on_command(id, code);
}

Brush *
WndForm::ClientAreaWindow::on_color(Window &window, Canvas &canvas)
{
#ifdef _WIN32_WCE
  if ((window.get_window_long(GWL_STYLE) & 0xf) == BS_PUSHBUTTON)
    /* Windows CE allows custom background colors for push buttons,
       while desktop Windows does not; to make push buttons retain
       their normal background color, we're implementing this
       exception */
    return ContainerWindow::on_color(window, canvas);
#endif

  canvas.set_text_color(Color::BLACK);
  canvas.set_background_color(background_color);
  return &background_brush;
}

void
WndForm::ClientAreaWindow::on_paint(Canvas &canvas)
{
  canvas.clear(background_brush);

  ContainerWindow::on_paint(canvas);
}

PeriodClock WndForm::timeAnyOpenClose;

static WindowStyle
add_border(WindowStyle style)
{
  style.border();
  return style;
}

WndForm::WndForm(SingleWindow &_main_window,
                 int X, int Y, int Width, int Height,
                 const TCHAR *Caption,
                 const WindowStyle style):
  main_window(_main_window),
  mModalResult(0),
  mColorTitle(Color::YELLOW),
  mhTitleFont(&Fonts::MapBold),
  mOnTimerNotify(NULL), mOnKeyDownNotify(NULL)
{
  _tcscpy(mCaption, Caption);

  set(main_window, X, Y, Width, Height, add_border(style));

  // Create ClientWindow

  SetBackColor(Color(0xDA, 0xDB, 0xAB));

  WindowStyle client_style;
  client_style.control_parent();
  client_area.set(*this, mClientRect.left, mClientRect.top,
                  mClientRect.right - mClientRect.left,
                  mClientRect.bottom - mClientRect.top, client_style);
  client_area.SetBackColor(GetBackColor());

#if !defined(ENABLE_SDL) && !defined(NDEBUG)
  ::SetWindowText(hWnd, mCaption);
#endif
}

void
WndForm::SetTimerNotify(TimerNotifyCallback_t OnTimerNotify)
{
  if (mOnTimerNotify != NULL && OnTimerNotify == NULL)
    kill_timer(cbTimerID);
  else if (mOnTimerNotify == NULL && OnTimerNotify != NULL)
    cbTimerID = set_timer(1001, 500);

  mOnTimerNotify = OnTimerNotify;
}

WndForm::~WndForm()
{
  /* we must override the ~Window() reset call, because in ~Window(),
     our own on_destroy() method won't be called (during object
     destruction, this object loses its identity) */
  reset();

  for (window_list_t::iterator i = destruct_windows.begin();
       i != destruct_windows.end(); ++i)
    delete *i;
}

void
WndForm::UpdateLayout()
{
  RECT rc = get_client_rect();

  mTitleRect = rc;
  mTitleRect.bottom = rc.top +
      (string_is_empty(mCaption) ? 0 : mhTitleFont->get_height());

  mClientRect = rc;
  mClientRect.top = mTitleRect.bottom;

  if (client_area.defined())
    client_area.move(mClientRect.left, mClientRect.top,
                     mClientRect.right - mClientRect.left,
                     mClientRect.bottom - mClientRect.top);
}

ContainerWindow &
WndForm::GetClientAreaWindow(void)
{
  return client_area;
}

Window *
WndForm::FindByName(const TCHAR *name)
{
  name_to_window_t::iterator i = name_to_window.find(name);
  if (i == name_to_window.end())
    return NULL;

  return i->second;
}

void
WndForm::FilterAdvanced(bool advanced)
{
  for (window_list_t::const_iterator i = advanced_windows.begin();
       i != advanced_windows.end(); ++i)
    (*i)->set_visible(advanced);
}

bool
WndForm::on_resize(unsigned width, unsigned height)
{
  ContainerWindow::on_resize(width, height);
  UpdateLayout();
  return true;
}

bool
WndForm::on_destroy()
{
  if (mModalResult == 0)
    mModalResult = mrCancel;

  if (mOnTimerNotify != NULL)
    kill_timer(cbTimerID);

  ContainerWindow::on_destroy();
  return true;
}

bool
WndForm::on_timer(timer_t id)
{
  if (id == cbTimerID) {
    if (mOnTimerNotify)
      mOnTimerNotify(*this);
    return true;
  } else
    return ContainerWindow::on_timer(id);
}

bool
WndForm::on_command(unsigned id, unsigned code)
{
  switch (id) {
  case IDCANCEL:
    /* sent by the WIN32 dialog manager when the user presses
       Escape */
    SetModalResult(mrCancel);
    return true;
  }

  return false;
}

void
WndForm::SetTitleFont(const Font &font)
{
  if (mhTitleFont != &font){
    // todo
    mhTitleFont = &font;

    invalidate();
    UpdateLayout();
  }
}

#ifndef ENABLE_SDL

/**
 * Is this key handled by the focused control? (bypassing the dialog
 * manager)
 */
gcc_pure
static bool
check_key(const MSG &msg)
{
  LRESULT r = ::SendMessage(msg.hwnd, WM_GETDLGCODE, msg.wParam,
                            (LPARAM)&msg);
  return (r & DLGC_WANTMESSAGE) != 0;
}

static bool
is_special_key(unsigned key_code)
{
  return key_code == VK_LEFT || key_code == VK_RIGHT ||
    key_code == VK_UP || key_code == VK_DOWN ||
    key_code == VK_TAB || key_code == VK_RETURN || key_code == VK_ESCAPE;
}

/**
 * Is this "special" key handled by the focused control? (bypassing
 * the dialog manager)
 */
gcc_pure
static bool
check_special_key(const MSG &msg)
{
  return is_special_key(msg.wParam) && check_key(msg);
}

#endif /* !ENABLE_SDL */

int
WndForm::ShowModal(Window *modal_allowed)
{
  assert_none_locked();

#define OPENCLOSESUPPRESSTIME 500
#ifndef ENABLE_SDL
  MSG msg;
  HWND oldFocusHwnd;
#endif /* !ENABLE_SDL */

  PeriodClock enter_clock;
  if (is_embedded() && !is_altair())
    enter_clock.update();

  show_on_top();

  mModalResult = 0;

#ifndef ENABLE_SDL
  oldFocusHwnd = ::GetFocus();
  if (oldFocusHwnd != NULL)
    ::SendMessage(oldFocusHwnd, WM_CANCELMODE, 0, 0);
#endif /* !ENABLE_SDL */
  set_focus();
  focus_first_control();

#ifndef ENABLE_SDL
  bool hastimed = false;
#endif /* !ENABLE_SDL */
  WndForm::timeAnyOpenClose.update(); // when current dlg opens or child closes

  main_window.add_dialog(this);

#ifdef ENABLE_SDL

  update();

#ifdef ANDROID
  EventLoop loop(*event_queue, main_window);
  Event event;
#else
  EventLoop loop(main_window);
  SDL_Event event;
#endif
  while (mModalResult == 0 && loop.get(event))
    if (main_window.FilterEvent(event, this, modal_allowed))
      loop.dispatch(event);

#else /* !ENABLE_SDL */

  DialogEventLoop loop(*this);
  while (mModalResult == 0 && loop.get(msg)) {
    if (!main_window.FilterEvent(msg, this, modal_allowed))
      continue;

    // hack to stop exiting immediately
    if (is_embedded() && !is_altair() && !hastimed &&
        is_user_input(msg.message)) {
      if (!enter_clock.check(200))
        /* ignore user input in the first 200ms */
        continue;
      else
        hastimed = true;
    }

    if (is_embedded() && msg.message == WM_LBUTTONUP &&
        !timeAnyOpenClose.check(OPENCLOSESUPPRESSTIME))
      /* prevents child click from being repeat-handled by parent if
         buttons overlap */
      continue;

    if (msg.message == WM_KEYDOWN && mOnKeyDownNotify != NULL &&
        identify_descendant(msg.hwnd) && !check_special_key(msg) &&
        mOnKeyDownNotify(*this, msg.wParam))
      continue;

    if (msg.message == WM_KEYDOWN && identify_descendant(msg.hwnd) &&
        (msg.wParam == VK_UP || msg.wParam == VK_DOWN)) {
      /* VK_UP and VK_DOWN move the focus only within the current
         control group - but we want it to behave like Shift-Tab and
         Tab */

      if (!check_key(msg)) {
        /* this window doesn't handle VK_UP/VK_DOWN */
        if (msg.wParam == VK_DOWN)
          focus_next_control();
        else
          focus_previous_control();
        continue;
      }
    }

    loop.dispatch(msg);
  } // End Modal Loop
#endif /* !ENABLE_SDL */

  main_window.remove_dialog(this);

  // static.  this is current open/close or child open/close
  WndForm::timeAnyOpenClose.update();

#ifndef ENABLE_SDL
  SetFocus(oldFocusHwnd);
#endif /* !ENABLE_SDL */

  return mModalResult;
}

void
WndForm::on_paint(Canvas &canvas)
{
  ContainerWindow::on_paint(canvas);

  // Get window coordinates
  RECT rcClient = get_client_rect();

  // Draw the borders
  canvas.raised_edge(rcClient);

  // Set the colors
  canvas.set_text_color(Color::BLACK);
  canvas.set_background_color(mColorTitle);
  canvas.background_transparent();

  // Set the titlebar font and font-size
  canvas.select(*mhTitleFont);

  // JMW todo add here icons?

  // Draw titlebar text
  canvas.text_opaque(mTitleRect.left + Layout::FastScale(2),
                     mTitleRect.top, mTitleRect, mCaption);
}

void
WndForm::SetCaption(const TCHAR *Value)
{
  if (Value == NULL)
    Value = _T("");

  if (_tcscmp(mCaption, Value) != 0){
    _tcscpy(mCaption, Value);
    UpdateLayout();
    invalidate(mTitleRect);
  }
}

void
WndForm::SetBackColor(Color Value)
{
  client_area.SetBackColor(Value);
}
