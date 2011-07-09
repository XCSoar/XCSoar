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
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Key.h"
#include "Util/StringUtil.hpp"
#include "Look/DialogLook.hpp"

#ifdef ENABLE_SDL
#include "Screen/SDL/Reference.hpp"
#endif

#ifdef ANDROID
#include "Screen/Android/Event.hpp"
#include "Android/Main.hpp"
#elif defined(ENABLE_SDL)
#include "Screen/SDL/Event.hpp"
#else
#include "Screen/GDI/Event.hpp"
#endif

#ifdef EYE_CANDY
#include "Screen/Bitmap.hpp"
#include "resource.h"
#endif

bool
WndForm::ClientAreaWindow::on_command(unsigned id, unsigned code)
{
  return (mCommandCallback != NULL && mCommandCallback(id))
    || ContainerWindow::on_command(id, code);
}

const Brush *
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

  canvas.set_text_color(COLOR_BLACK);
  canvas.set_background_color(look.background_color);
  return &look.background_brush;
}

void
WndForm::ClientAreaWindow::on_paint(Canvas &canvas)
{
  canvas.clear(look.background_color);

  ContainerWindow::on_paint(canvas);
}

PeriodClock WndForm::timeAnyOpenClose;

static WindowStyle
add_border(WindowStyle style)
{
  style.border();
  return style;
}

WndForm::WndForm(SingleWindow &_main_window, const DialogLook &_look,
                 int X, int Y, int Width, int Height,
                 const TCHAR *Caption,
                 const WindowStyle style):
  main_window(_main_window), look(_look),
  mModalResult(0), force(false),
  mhTitleFont(&Fonts::MapBold),
  client_area(_look),
  mOnTimerNotify(NULL), mOnKeyDownNotify(NULL)
{
  mCaption = Caption;

  set(main_window, X, Y, Width, Height, add_border(style));

  // Create ClientWindow

#ifdef EYE_CANDY
  bitmap_title.load(IDB_DIALOGTITLE);
#endif

  WindowStyle client_style;
  client_style.control_parent();
  client_area.set(*this, mClientRect.left, mClientRect.top,
                  mClientRect.right - mClientRect.left,
                  mClientRect.bottom - mClientRect.top, client_style);

#if !defined(ENABLE_SDL) && !defined(NDEBUG)
  ::SetWindowText(hWnd, mCaption.c_str());
#endif
}

void
WndForm::SetTimerNotify(TimerNotifyCallback_t OnTimerNotify, unsigned ms)
{
  if (mOnTimerNotify != NULL && OnTimerNotify == NULL)
    kill_timer(cbTimerID);
  else if (mOnTimerNotify == NULL && OnTimerNotify != NULL)
    cbTimerID = set_timer(1001, ms);

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
  PixelRect rc = get_client_rect();

  mTitleRect = rc;
  mTitleRect.bottom = rc.top +
    (mCaption.empty() ? 0 : mhTitleFont->get_height());

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
  for (window_list_t::const_iterator i = expert_windows.begin();
       i != expert_windows.end(); ++i)
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

static bool
is_special_key(unsigned key_code)
{
  return key_code == VK_LEFT || key_code == VK_RIGHT ||
    key_code == VK_UP || key_code == VK_DOWN ||
    key_code == VK_TAB || key_code == VK_RETURN || key_code == VK_ESCAPE;
}

#ifdef ANDROID

static bool
is_key_down(const Event &event)
{
  return event.type == Event::KEY_DOWN;
}

static unsigned
get_key_code(const Event &event)
{
  assert(event.type == Event::KEY_DOWN || event.type == Event::KEY_UP);

  return event.param;
}

static bool
is_mouse_up(const Event &event)
{
  return event.type == Event::MOUSE_UP;
}

gcc_pure
static bool
check_key(ContainerWindow *container, const Event &event)
{
  Window *focused = container->get_focused_window();
  if (focused == NULL)
    return false;

  return focused->on_key_check(get_key_code(event));
}

gcc_pure
static bool
check_special_key(ContainerWindow *container, const Event &event)
{
  return is_special_key(get_key_code(event)) && check_key(container, event);
}

#elif defined(ENABLE_SDL)

static bool
is_key_down(const SDL_Event &event)
{
  return event.type == SDL_KEYDOWN;
}

static unsigned
get_key_code(const SDL_Event &event)
{
  assert(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP);

  return event.key.keysym.sym;
}

static bool
is_mouse_up(const SDL_Event &event)
{
  return event.type == SDL_MOUSEBUTTONUP;
}

gcc_pure
static bool
check_key(ContainerWindow *container, const SDL_Event &event)
{
  Window *focused = container->get_focused_window();
  if (focused == NULL)
    return false;

  return focused->on_key_check(get_key_code(event));
}

gcc_pure
static bool
check_special_key(ContainerWindow *container, const SDL_Event &event)
{
  return is_special_key(get_key_code(event)) && check_key(container, event);
}

#else /* GDI follows: */

static bool
is_key_down(const MSG &msg)
{
  return msg.message == WM_KEYDOWN;
}

static unsigned
get_key_code(const MSG &msg)
{
  assert(msg.message == WM_KEYDOWN || msg.message == WM_KEYUP);

  return msg.wParam;
}

static bool
is_mouse_up(const MSG &msg)
{
  return msg.message == WM_LBUTTONUP;
}

/**
 * Is this key handled by the focused control? (bypassing the dialog
 * manager)
 */
gcc_pure
static bool
check_key(ContainerWindow *container, const MSG &msg)
{
  LRESULT r = ::SendMessage(msg.hwnd, WM_GETDLGCODE, msg.wParam,
                            (LPARAM)&msg);
  return (r & DLGC_WANTMESSAGE) != 0;
}

/**
 * Is this "special" key handled by the focused control? (bypassing
 * the dialog manager)
 */
gcc_pure
static bool
check_special_key(ContainerWindow *container, const MSG &msg)
{
  return is_special_key(msg.wParam) && check_key(container, msg);
}

#endif /* !ENABLE_SDL */

int
WndForm::ShowModal(Window *mouse_allowed)
{
  assert_none_locked();

#define OPENCLOSESUPPRESSTIME 500
#ifdef ENABLE_SDL
  ContainerWindow *root = get_root_owner();
  WindowReference old_focus_reference = root->GetFocusedWindowReference();
#else
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

  bool hastimed = false;
  WndForm::timeAnyOpenClose.update(); // when current dlg opens or child closes

  main_window.add_dialog(this);

#ifdef ENABLE_SDL
  main_window.refresh();
#endif

#ifdef ANDROID
  EventLoop loop(*event_queue, main_window);
  Event event;
#elif defined(ENABLE_SDL)
  EventLoop loop(main_window);
  SDL_Event event;
#else
  DialogEventLoop loop(*this);
  MSG event;
#endif

  while ((mModalResult == 0 || force) && loop.get(event)) {
    if (!main_window.FilterEvent(event, this, mouse_allowed))
      continue;

    // hack to stop exiting immediately
    if (is_embedded() && !is_altair() && !hastimed &&
        is_user_input(event)) {
      if (!enter_clock.check(200))
        /* ignore user input in the first 200ms */
        continue;
      else
        hastimed = true;
    }

    if (is_embedded() && is_mouse_up(event) &&
        !timeAnyOpenClose.check(OPENCLOSESUPPRESSTIME))
      /* prevents child click from being repeat-handled by parent if
         buttons overlap */
      continue;

    if (mOnKeyDownNotify != NULL && is_key_down(event) &&
#ifndef ENABLE_SDL
        identify_descendant(event.hwnd) &&
#endif
        !check_special_key(this, event) &&
        mOnKeyDownNotify(*this, get_key_code(event)))
      continue;

#if defined(ENABLE_SDL) && !defined(ANDROID)
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_TAB) {
      /* the Tab key moves the keyboard focus */
      const Uint8 *keystate = ::SDL_GetKeyState(NULL);
      event.key.keysym.sym = keystate[SDLK_LSHIFT] || keystate[SDLK_RSHIFT]
        ? SDLK_UP : SDLK_DOWN;
    }
#endif

    if (is_key_down(event) &&
#ifndef ENABLE_SDL
        identify_descendant(event.hwnd) &&
#endif
        (get_key_code(event) == VK_UP || get_key_code(event) == VK_DOWN)) {
      /* VK_UP and VK_DOWN move the focus only within the current
         control group - but we want it to behave like Shift-Tab and
         Tab */

      if (!check_key(this, event)) {
        /* this window doesn't handle VK_UP/VK_DOWN */
        if (get_key_code(event) == VK_DOWN)
          focus_next_control();
        else
          focus_previous_control();
        continue;
      }
    }

#ifdef ENABLE_SDL
    if (is_key_down(event) && get_key_code(event) == VK_ESCAPE) {
      mModalResult = mrCancel;
      continue;
    }
#endif

    /* map VK_ESCAPE to mrOK on Altair, because the Escape key is expected to 
       be the one that saves and closes a dialog */
    if (is_altair() && is_key_down(event) && get_key_code(event) == VK_ESCAPE) {
      mModalResult = mrOK;
      continue;
    }

    loop.dispatch(event);
  } // End Modal Loop

  main_window.remove_dialog(this);

  // static.  this is current open/close or child open/close
  WndForm::timeAnyOpenClose.update();

#ifdef ENABLE_SDL
  if (old_focus_reference.Defined()) {
    Window *old_focus = old_focus_reference.Get(*root);
    if (old_focus != NULL)
      old_focus->set_focus();
  }
#else
  SetFocus(oldFocusHwnd);
#endif /* !ENABLE_SDL */

  return mModalResult;
}

void
WndForm::on_paint(Canvas &canvas)
{
  ContainerWindow::on_paint(canvas);

  // Get window coordinates
  PixelRect rcClient = get_client_rect();

  // Draw the borders
  canvas.raised_edge(rcClient);

  if (!mCaption.empty()) {
    // Set the colors
    canvas.set_text_color(COLOR_WHITE);

    // Set the titlebar font and font-size
    canvas.select(*mhTitleFont);

    // JMW todo add here icons?

#ifdef EYE_CANDY
    canvas.background_transparent();
    canvas.stretch(mTitleRect.left, mTitleRect.top,
                   mTitleRect.right - mTitleRect.left,
                   mTitleRect.bottom - mTitleRect.top,
                   bitmap_title);

    // Draw titlebar text
    canvas.text(mTitleRect.left + Layout::FastScale(2), mTitleRect.top,
                mCaption.c_str());
#else
    canvas.set_background_color(look.caption.background_color);
    canvas.text_opaque(mTitleRect.left + Layout::FastScale(2),
                       mTitleRect.top, mTitleRect, mCaption.c_str());
#endif
  }
}

void
WndForm::SetCaption(const TCHAR *Value)
{
  if (Value == NULL)
    Value = _T("");

  if (!mCaption.equals(Value)){
    mCaption = Value;
    UpdateLayout();
    invalidate(mTitleRect);
  }
}

#ifdef ANDROID
void
WndForm::ReinitialiseLayout()
{
  if (main_window.get_width() < get_width() ||
      main_window.get_height() < get_height()) {
    // close dialog, it's creator may want to create a new layout
    mModalResult = mrChangeLayout;
  } else {
    // reposition dialog to fit into TopWindow
    int left = Window::get_left();
    int top = Window::get_top();

    if (Window::get_right() > (int) main_window.get_width())
      left = main_window.get_width() - get_width();
    if (Window::get_bottom() > (int) main_window.get_height())
      top = main_window.get_height() - get_height();

    if (left != Window::get_left() || top != Window::get_top())
      move(left, top);
  }
}
#endif
