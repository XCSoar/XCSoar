/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Time/PeriodClock.hpp"
#include "Asset.hpp"
#include "Interface.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"
#include "Look/DialogLook.hpp"

#ifndef USE_GDI
#include "Screen/Custom/Reference.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Event/Shared/Event.hpp"
#include "Event/Android/Loop.hpp"
#elif defined(ENABLE_SDL)
#include "Event/SDL/Event.hpp"
#include "Event/SDL/Loop.hpp"
#elif defined(USE_CONSOLE)
#include "Event/Console/Globals.hpp"
#include "Event/Shared/Event.hpp"
#include "Event/Console/Loop.hpp"
#elif defined(USE_GDI)
#include "Event/GDI/Event.hpp"
#include "Event/GDI/Loop.hpp"
#endif

#ifdef USE_GDI

const Brush *
WndForm::ClientAreaWindow::OnChildColor(Window &window, Canvas &canvas)
{
#ifdef _WIN32_WCE
  if ((window.GetStyle() & 0xf) == BS_PUSHBUTTON)
    /* Windows CE allows custom background colors for push buttons,
       while desktop Windows does not; to make push buttons retain
       their normal background color, we're implementing this
       exception */
    return ContainerWindow::OnChildColor(window, canvas);
#endif

  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(look.background_color);
  return &look.background_brush;
}

#endif

void
WndForm::ClientAreaWindow::OnPaint(Canvas &canvas)
{
  canvas.Clear(look.background_color);

  ContainerWindow::OnPaint(canvas);
}

static WindowStyle
AddBorder(WindowStyle style)
{
  style.Border();
  return style;
}

WndForm::WndForm(const DialogLook &_look)
  :look(_look),
   modal_result(0), force(false),
   modeless(false),
   dragging(false),
   client_area(_look),
   default_focus(nullptr)
{
}

WndForm::WndForm(SingleWindow &main_window, const DialogLook &_look,
                 const PixelRect &rc,
                 const TCHAR *Caption,
                 const WindowStyle style)
  :look(_look),
   modal_result(0), force(false),
   modeless(false),
   dragging(false),
   client_area(_look),
   default_focus(NULL)
{
  Create(main_window, rc, Caption, AddBorder(style));
}

void
WndForm::Create(SingleWindow &main_window, const PixelRect &rc,
                const TCHAR *_caption, const WindowStyle style)
{
  if (_caption != nullptr)
    caption = _caption;
  else
    caption.clear();

  ContainerWindow::Create(main_window, rc, AddBorder(style));

#if defined(USE_GDI) && !defined(NDEBUG)
  ::SetWindowText(hWnd, caption.c_str());
#endif
}

void
WndForm::Create(SingleWindow &main_window,
                const TCHAR *_caption, const WindowStyle style)
{
  Create(main_window, main_window.GetClientRect(), _caption, style);
}

WndForm::~WndForm()
{
  /* we must override the ~Window() reset call, because in ~Window(),
     our own OnDestroy() method won't be called (during object
     destruction, this object loses its identity) */
  Destroy();
  SubForm::Clear();
}

SingleWindow &
WndForm::GetMainWindow()
{
  return *(SingleWindow *)GetRootOwner();
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
}

void
WndForm::OnCreate()
{
  ContainerWindow::OnCreate();

  UpdateLayout();

  WindowStyle client_style;
  client_style.ControlParent();
  client_area.Create(*this, client_rect, client_style);
}

void
WndForm::OnResize(PixelSize new_size)
{
  ContainerWindow::OnResize(new_size);
  UpdateLayout();
  client_area.Move(client_rect);
}

void
WndForm::OnDestroy()
{
  if (modal_result == 0)
    modal_result = mrCancel;

  ContainerWindow::OnDestroy();
}

bool
WndForm::OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys)
{
  if (ContainerWindow::OnMouseMove(x, y, keys))
    return true;

  if (dragging) {
    const PixelRect position = GetPosition();
    const int dx = position.left + x - last_drag.x;
    const int dy = position.top + y - last_drag.y;
    last_drag.x = position.left + x;
    last_drag.y = position.top + y;

    PixelRect parent = GetParentClientRect();
    parent.Grow(-client_rect.top);

    PixelRect new_position = position;
    new_position.Offset(dx, dy);

    if (new_position.right < parent.left)
      new_position.Offset(parent.left - new_position.right, 0);

    if (new_position.left > parent.right)
      new_position.Offset(parent.right - new_position.left, 0);

    if (new_position.top > parent.bottom)
      new_position.Offset(0, parent.bottom - new_position.top);

    if (new_position.top < 0)
      new_position.Offset(0, -new_position.top);

    Move(new_position.left, new_position.top);

    return true;
  }

  return false;
}

bool
WndForm::OnMouseDown(PixelScalar x, PixelScalar y)
{
  if (ContainerWindow::OnMouseDown(x, y))
    return true;

  if (!dragging && !IsMaximised()) {
    dragging = true;
    Invalidate();

    const PixelRect position = GetPosition();
    last_drag.x = position.left + x;
    last_drag.y = position.top + y;
    SetCapture();
    return true;
  }

  return false;
}

bool
WndForm::OnMouseUp(PixelScalar x, PixelScalar y)
{
  if (ContainerWindow::OnMouseUp(x, y))
    return true;

  if (dragging) {
    dragging = false;
    Invalidate();
    ReleaseCapture();
    return true;
  }

  return false;
}

void
WndForm::OnCancelMode()
{
  ContainerWindow::OnCancelMode();

  if (dragging) {
    dragging = false;
    Invalidate();
    ReleaseCapture();
  }
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
IsSpecialKey(unsigned key_code)
{
  return key_code == KEY_LEFT || key_code == KEY_RIGHT ||
    key_code == KEY_UP || key_code == KEY_DOWN ||
    key_code == KEY_TAB || key_code == KEY_RETURN || key_code == KEY_ESCAPE;
}

/**
 * Is this key handled by the focused control? (bypassing the dialog
 * manager)
 */
gcc_pure
static bool
CheckKey(ContainerWindow *container, const Event &event)
{
#ifdef USE_GDI
  const MSG &msg = event.msg;
  LRESULT r = ::SendMessage(msg.hwnd, WM_GETDLGCODE, msg.wParam,
                            (LPARAM)&msg);
  return (r & DLGC_WANTMESSAGE) != 0;
#else
  Window *focused = container->GetFocusedWindow();
  if (focused == NULL)
    return false;

  return focused->OnKeyCheck(event.GetKeyCode());
#endif
}

/**
 * Is this "special" key handled by the focused control? (bypassing
 * the dialog manager)
 */
gcc_pure
static bool
CheckSpecialKey(ContainerWindow *container, const Event &event)
{
  return IsSpecialKey(event.GetKeyCode()) && CheckKey(container, event);
}

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

  SingleWindow &main_window = GetMainWindow();
  main_window.CancelMode();

#ifdef USE_GDI
  oldFocusHwnd = ::GetFocus();
#endif /* USE_GDI */
  SetFocus();
  if (default_focus)
    default_focus->SetFocus();
  else
    client_area.FocusFirstControl();

  bool hastimed = false;

  main_window.AddDialog(this);

#ifndef USE_GDI
  main_window.Refresh();
#endif

#if defined(ANDROID) || defined(USE_CONSOLE)
  EventLoop loop(*event_queue, main_window);
#elif defined(ENABLE_SDL)
  EventLoop loop(main_window);
#else
  DialogEventLoop loop(*this);
#endif
  Event event;

  while ((modal_result == 0 || force) && loop.Get(event)) {
    if (!main_window.FilterEvent(event, this)) {
      if (modeless && event.IsMouseDown())
        break;
      else
        continue;
    }

    // hack to stop exiting immediately
    if (IsEmbedded() && !IsAltair() && !hastimed &&
        event.IsUserInput()) {
      if (!enter_clock.Check(200))
        /* ignore user input in the first 200ms */
        continue;
      else
        hastimed = true;
    }

    if (event.IsKeyDown()) {
      if (
#ifdef USE_GDI
          IdentifyDescendant(event.msg.hwnd) &&
#endif
          !CheckSpecialKey(this, event) &&
          OnAnyKeyDown(event.GetKeyCode()))
        continue;

#ifdef ENABLE_SDL
      if (event.GetKeyCode() == SDLK_TAB) {
        /* the Tab key moves the keyboard focus */
        const Uint8 *keystate = ::SDL_GetKeyState(NULL);
        event.event.key.keysym.sym =
          keystate[SDLK_LSHIFT] || keystate[SDLK_RSHIFT]
          ? SDLK_UP : SDLK_DOWN;
      }
#endif

      if (
#ifdef USE_GDI
          IdentifyDescendant(event.msg.hwnd) &&
#endif
          (event.GetKeyCode() == KEY_UP || event.GetKeyCode() == KEY_DOWN)) {
        /* KEY_UP and KEY_DOWN move the focus only within the current
           control group - but we want it to behave like Shift-Tab and
           Tab */

        if (!CheckKey(this, event)) {
          /* this window doesn't handle KEY_UP/KEY_DOWN */
          if (event.GetKeyCode() == KEY_DOWN)
            FocusNextControl();
          else
            FocusPreviousControl();
          continue;
        }
      }

#if !defined USE_GDI || defined _WIN32_WCE
      /* The Windows CE dialog manager does not handle KEY_ESCAPE and
         so we have to do it by ourself */

      // On Altair, the RemoteKey ("E" Button) shall also close the analyse-page
      if (IsAltair()) {
#ifdef GNAV
        if (event.GetKeyCode() == KEY_ESCAPE || event.GetKeyCode() == KEY_F15) {
          modal_result = mrOK;
          continue;
        }
#endif
      } else if (event.GetKeyCode() == KEY_ESCAPE) {
        modal_result = mrCancel;
        continue;
      }
#endif

#ifdef KOBO
#ifdef ENABLE_SDL
      if (event.GetKeyCode() == SDLK_POWER) {
        /* the Kobo power button closes the modal dialog */
        modal_result = mrCancel;
        continue;
      }
#else
  // TODO: check the console key code
#endif
#endif
    }

    if (event.IsCharacter() && character_function &&
        character_function(event.GetCharacter()))
      continue;

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
  const SingleWindow &main_window = GetMainWindow();
  gcc_unused const bool is_active = main_window.IsTopDialog(*this);

#ifdef ENABLE_OPENGL
  if (!IsMaximised() && is_active) {
    /* draw a shade around the current dialog to emphasise it */
    const GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnableClientState(GL_COLOR_ARRAY);

    const PixelRect rc = GetClientRect();
    const PixelScalar size = Layout::SmallScale(4);

    const RasterPoint vertices[8] = {
      { rc.left, rc.top },
      { rc.right, rc.top },
      { rc.right, rc.bottom },
      { rc.left, rc.bottom },
      { rc.left - size, rc.top - size },
      { rc.right + size, rc.top - size },
      { rc.right + size, rc.bottom + size },
      { rc.left - size, rc.bottom + size },
    };

    glVertexPointer(2, GL_VALUE, 0, vertices);

    static constexpr Color inner_color = COLOR_BLACK.WithAlpha(192);
    static constexpr Color outer_color = COLOR_BLACK.WithAlpha(16);
    static constexpr Color colors[8] = {
      inner_color,
      inner_color,
      inner_color,
      inner_color,
      outer_color,
      outer_color,
      outer_color,
      outer_color,
    };

#ifdef HAVE_GLES
    glColorPointer(4, GL_FIXED, 0, colors);
#else
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
#endif

    static constexpr GLubyte indices[] = {
      0, 4, 1, 4, 5, 1,
      1, 5, 2, 5, 6, 2,
      2, 6, 3, 6, 7, 3,
      3, 7, 0, 7, 4, 0,
    };

    glDrawElements(GL_TRIANGLES, ARRAY_SIZE(indices),
                   GL_UNSIGNED_BYTE, indices);

    glDisableClientState(GL_COLOR_ARRAY);
  }
#endif

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
    canvas.DrawText(title_rect.left + Layout::FastScale(2), title_rect.top,
                    caption.c_str());
#else
    canvas.SetBackgroundColor(is_active
                              ? look.caption.background_color
                              : look.caption.inactive_background_color);
    canvas.DrawOpaqueText(title_rect.left + Layout::FastScale(2),
                          title_rect.top, title_rect, caption.c_str());
#endif
  }

  if (dragging) {
#ifdef ENABLE_OPENGL
    const GLBlend blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    canvas.DrawFilledRectangle(0, 0, canvas.GetWidth(), canvas.GetHeight(),
                               COLOR_YELLOW.WithAlpha(80));
#elif defined(USE_GDI)
    ::InvertRect(canvas, &title_rect);
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
    client_area.Move(client_rect);
    Invalidate(title_rect);
  }
}

#ifdef ANDROID
void
WndForm::ReinitialiseLayout()
{
  const SingleWindow &main_window = GetMainWindow();

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

bool
WndForm::OnAnyKeyDown(unsigned key_code)
{
  return key_down_function && key_down_function(key_code);
}
