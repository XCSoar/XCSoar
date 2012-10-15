/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "PeriodClock.hpp"
#include "Asset.hpp"
#include "Interface.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Event.hpp"
#include "Util/StringUtil.hpp"
#include "Look/DialogLook.hpp"

#ifndef USE_GDI
#include "Screen/Custom/Reference.hpp"
#endif

#ifdef ANDROID
#include "Android/Main.hpp"
#endif

bool
WndForm::ClientAreaWindow::OnCommand(unsigned id, unsigned code)
{
  return (command_callback != NULL && command_callback(id))
    || ContainerWindow::OnCommand(id, code);
}

const Brush *
WndForm::ClientAreaWindow::on_color(Window &window, Canvas &canvas)
{
#ifdef _WIN32_WCE
  if ((window.GetStyle() & 0xf) == BS_PUSHBUTTON)
    /* Windows CE allows custom background colors for push buttons,
       while desktop Windows does not; to make push buttons retain
       their normal background color, we're implementing this
       exception */
    return ContainerWindow::on_color(window, canvas);
#endif

  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(look.background_color);
  return &look.background_brush;
}

void
WndForm::ClientAreaWindow::OnPaint(Canvas &canvas)
{
  canvas.Clear(look.background_color);

  ContainerWindow::OnPaint(canvas);
}

static WindowStyle
add_border(WindowStyle style)
{
  style.Border();
  return style;
}

WndForm::WndForm(SingleWindow &_main_window, const DialogLook &_look,
                 const PixelRect &rc,
                 const TCHAR *Caption,
                 const WindowStyle style)
  :main_window(_main_window), look(_look),
   modal_result(0), force(false),
   modeless(false),
   client_area(_look),
   timer_notify_callback(NULL), key_down_notify_callback(NULL),
   default_focus(NULL),
   timer(*this)
{
  caption = Caption;

  Create(main_window, rc, add_border(style));

  // Create ClientWindow

  WindowStyle client_style;
  client_style.ControlParent();
  client_area.Create(*this, client_rect, client_style);

#if defined(USE_GDI) && !defined(NDEBUG)
  ::SetWindowText(hWnd, caption.c_str());
#endif
}

void
WndForm::SetTimerNotify(TimerNotifyCallback OnTimerNotify, unsigned ms)
{
  if (timer_notify_callback != NULL && OnTimerNotify == NULL)
    timer.Cancel();
  else if (timer_notify_callback == NULL && OnTimerNotify != NULL)
    timer.Schedule(ms);

  timer_notify_callback = OnTimerNotify;
}

WndForm::~WndForm()
{
  /* we must override the ~Window() reset call, because in ~Window(),
     our own OnDestroy() method won't be called (during object
     destruction, this object loses its identity) */
  Destroy();
  SubForm::Clear();
}

void
WndForm::UpdateLayout()
{
  PixelRect rc = GetClientRect();

  title_rect = rc;
  title_rect.bottom = rc.top +
    (caption.empty() ? 0 : look.caption.font->GetHeight());

  client_rect = rc;
  client_rect.top = title_rect.bottom;

  if (client_area.IsDefined())
    client_area.Move(client_rect);
}

ContainerWindow &
WndForm::GetClientAreaWindow()
{
  return client_area;
}

void
WndForm::OnResize(UPixelScalar width, UPixelScalar height)
{
  ContainerWindow::OnResize(width, height);
  UpdateLayout();
}

void
WndForm::OnDestroy()
{
  if (modal_result == 0)
    modal_result = mrCancel;

  timer.Cancel();

  ContainerWindow::OnDestroy();
}

bool
WndForm::OnTimer(WindowTimer &_timer)
{
  if (_timer == timer) {
    if (timer_notify_callback)
      timer_notify_callback(*this);
    return true;
  } else
    return ContainerWindow::OnTimer(_timer);
}

#ifdef WIN32

bool
WndForm::OnCommand(unsigned id, unsigned code)
{
  switch (id) {
  case IDCANCEL:
    /* sent by the WIN32 dialog manager when the user presses
       Escape */
    SetModalResult(mrCancel);
    return true;
  }

  return ContainerWindow::OnCommand(id, code);
}

#endif

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
is_mouse_down(const Event &event)
{
  return event.type == Event::MOUSE_DOWN;
}

gcc_pure
static bool
check_key(ContainerWindow *container, const Event &event)
{
  Window *focused = container->GetFocusedWindow();
  if (focused == NULL)
    return false;

  return focused->OnKeyCheck(get_key_code(event));
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
is_mouse_down(const SDL_Event &event)
{
  return event.type == SDL_MOUSEBUTTONDOWN;
}

gcc_pure
static bool
check_key(ContainerWindow *container, const SDL_Event &event)
{
  Window *focused = container->GetFocusedWindow();
  if (focused == NULL)
    return false;

  return focused->OnKeyCheck(get_key_code(event));
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
is_mouse_down(const MSG &msg)
{
  return msg.message == WM_LBUTTONDOWN;
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

int WndForm::ShowModeless()
{
  modeless = true;
  return ShowModal();
}

int
WndForm::ShowModal()
{
  AssertNoneLocked();

#define OPENCLOSESUPPRESSTIME 500
#ifndef USE_GDI
  ContainerWindow *root = GetRootOwner();
  WindowReference old_focus_reference = root->GetFocusedWindowReference();
#else
  HWND oldFocusHwnd;
#endif /* USE_GDI */

  PeriodClock enter_clock;
  if (IsEmbedded() && !IsAltair())
    enter_clock.Update();

  ShowOnTop();

  modal_result = 0;

  main_window.CancelMode();

#ifdef USE_GDI
  oldFocusHwnd = ::GetFocus();
#endif /* USE_GDI */
  SetFocus();
  if (default_focus)
    default_focus->SetFocus();
  else
    FocusFirstControl();

  bool hastimed = false;

  main_window.AddDialog(this);

#ifndef USE_GDI
  main_window.Refresh();
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

  while ((modal_result == 0 || force) && loop.Get(event)) {
    if (!main_window.FilterEvent(event, this)) {
      if (modeless && is_mouse_down(event))
        break;
      else
        continue;
    }

    // hack to stop exiting immediately
    if (IsEmbedded() && !IsAltair() && !hastimed &&
        IsUserInput(event)) {
      if (!enter_clock.Check(200))
        /* ignore user input in the first 200ms */
        continue;
      else
        hastimed = true;
    }

    if (key_down_notify_callback != NULL && is_key_down(event) &&
#ifdef USE_GDI
        IdentifyDescendant(event.hwnd) &&
#endif
        !check_special_key(this, event) &&
        key_down_notify_callback(*this, get_key_code(event)))
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
#ifdef USE_GDI
        IdentifyDescendant(event.hwnd) &&
#endif
        (get_key_code(event) == VK_UP || get_key_code(event) == VK_DOWN)) {
      /* VK_UP and VK_DOWN move the focus only within the current
         control group - but we want it to behave like Shift-Tab and
         Tab */

      if (!check_key(this, event)) {
        /* this window doesn't handle VK_UP/VK_DOWN */
        if (get_key_code(event) == VK_DOWN)
          FocusNextControl();
        else
          FocusPreviousControl();
        continue;
      }
    }

#if !defined USE_GDI || defined _WIN32_WCE
    // The Windows CE dialog manager does not handle VK_ESCAPE and so we have
    // to do it by ourself.
    if (is_key_down(event) && get_key_code(event) == VK_ESCAPE) {
      modal_result = mrCancel;
      continue;
    }
#endif

    /* map VK_ESCAPE to mrOK on Altair, because the Escape key is expected to 
       be the one that saves and closes a dialog */
    if (IsAltair() && is_key_down(event) && get_key_code(event) == VK_ESCAPE) {
      modal_result = mrOK;
      continue;
    }

    loop.Dispatch(event);
  } // End Modal Loop

  main_window.RemoveDialog(this);

#ifdef USE_GDI
  ::SetFocus(oldFocusHwnd);
#else
  if (old_focus_reference.Defined()) {
    Window *old_focus = old_focus_reference.Get(*root);
    if (old_focus != NULL)
      old_focus->SetFocus();
  }
#endif /* !USE_GDI */

  return modal_result;
}

void
WndForm::OnPaint(Canvas &canvas)
{
  ContainerWindow::OnPaint(canvas);

  // Get window coordinates
  PixelRect rcClient = GetClientRect();

  // Draw the borders
  canvas.DrawRaisedEdge(rcClient);

  if (!caption.empty()) {
    // Set the colors
    canvas.SetTextColor(COLOR_WHITE);

    // Set the titlebar font and font-size
    canvas.Select(*look.caption.font);

    // JMW todo add here icons?

#ifdef EYE_CANDY
    canvas.SetBackgroundTransparent();
    canvas.Stretch(title_rect.left, title_rect.top,
                   title_rect.right - title_rect.left,
                   title_rect.bottom - title_rect.top,
                   look.caption.background_bitmap);

    // Draw titlebar text
    canvas.text(title_rect.left + Layout::FastScale(2), title_rect.top,
                caption.c_str());
#else
    canvas.SetBackgroundColor(main_window.IsTopDialog(*this)
                              ? look.caption.background_color
                              : look.caption.inactive_background_color);
    canvas.text_opaque(title_rect.left + Layout::FastScale(2),
                       title_rect.top, title_rect, caption.c_str());
#endif
  }
}

void
WndForm::SetCaption(const TCHAR *_caption)
{
  if (_caption == NULL)
    _caption = _T("");

  if (!caption.equals(_caption)) {
    caption = _caption;
    UpdateLayout();
    Invalidate(title_rect);
  }
}

#ifdef ANDROID
void
WndForm::ReinitialiseLayout()
{
  if (main_window.GetWidth() < GetWidth() ||
      main_window.GetHeight() < GetHeight()) {
    // close dialog, it's creator may want to create a new layout
    modal_result = mrChangeLayout;
  } else {
    // reposition dialog to fit into TopWindow
    PixelScalar left = GetLeft();
    PixelScalar top = GetTop();

    if (GetRight() > (PixelScalar) main_window.GetWidth())
      left = main_window.GetWidth() - GetWidth();
    if (GetBottom() > (PixelScalar) main_window.GetHeight())
      top = main_window.GetHeight() - GetHeight();

    if (left != GetLeft() || top != GetTop())
      Move(left, top);
  }
}
#endif
