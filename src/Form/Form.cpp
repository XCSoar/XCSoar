/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Screen/Canvas.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Event/KeyCode.hpp"
#include "Util/Macros.hpp"
#include "Look/DialogLook.hpp"
#include "Event/Globals.hpp"

#ifndef USE_WINUSER
#include "Screen/Custom/Reference.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/VertexPointer.hpp"
#endif

#ifdef ANDROID
#include "Event/Shared/Event.hpp"
#include "Event/Android/Loop.hpp"
#elif defined(ENABLE_SDL)
#include "Event/SDL/Event.hpp"
#include "Event/SDL/Loop.hpp"
#elif defined(USE_POLL_EVENT)
#include "Event/Shared/Event.hpp"
#include "Event/Poll/Loop.hpp"
#elif defined(WIN32)
#include "Event/Windows/Event.hpp"
#include "Event/Windows/Loop.hpp"
#endif

static WindowStyle
AddBorder(WindowStyle style)
{
  style.Border();
  return style;
}

WndForm::WndForm(const DialogLook &_look)
  :look(_look)
{
}

WndForm::WndForm(SingleWindow &main_window, const DialogLook &_look,
                 const PixelRect &rc,
                 const TCHAR *Caption,
                 const WindowStyle style)
  :look(_look)
{
  Create(main_window, rc, Caption, style);
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

#if defined(USE_WINUSER) && !defined(NDEBUG)
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
  client_area.Create(*this, client_rect, look.background_color, client_style);
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
WndForm::OnMouseMove(PixelPoint p, unsigned keys)
{
  if (ContainerWindow::OnMouseMove(p, keys))
    return true;

  if (dragging) {
    const PixelRect position = GetPosition();
    const int dx = position.left + p.x - last_drag.x;
    const int dy = position.top + p.y - last_drag.y;
    last_drag.x = position.left + p.x;
    last_drag.y = position.top + p.y;

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

#ifdef USE_MEMORY_CANVAS
    /* the RasterCanvas class doesn't clip negative window positions
       properly, therefore we avoid this problem at this stage */
    if (new_position.left < 0)
      new_position.left = 0;

    if (new_position.top < 0)
      new_position.top = 0;
#endif

    Move(new_position.left, new_position.top);

    return true;
  }

  return false;
}

bool
WndForm::OnMouseDown(PixelPoint p)
{
  if (ContainerWindow::OnMouseDown(p))
    return true;

  if (!dragging && !IsMaximised()) {
    dragging = true;
    Invalidate();

    const PixelRect position = GetPosition();
    last_drag.x = position.left + p.x;
    last_drag.y = position.top + p.y;
    SetCapture();
    return true;
  }

  return false;
}

bool
WndForm::OnMouseUp(PixelPoint p)
{
  if (ContainerWindow::OnMouseUp(p))
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

/**
 * Is this key handled by the focused control? (bypassing the dialog
 * manager)
 */
gcc_pure
static bool
CheckKey(ContainerWindow *container, const Event &event)
{
#ifdef USE_WINUSER
  const MSG &msg = event.msg;
  LRESULT r = ::SendMessage(msg.hwnd, WM_GETDLGCODE, msg.wParam,
                            (LPARAM)&msg);
  return (r & DLGC_WANTMESSAGE) != 0;
#else
  Window *focused = container->GetFocusedWindow();
  if (focused == nullptr)
    return false;

  return focused->OnKeyCheck(event.GetKeyCode());
#endif
}

int
WndForm::ShowModal()
{
#ifndef USE_WINUSER
  ContainerWindow *root = GetRootOwner();
  WindowReference old_focus_reference = root->GetFocusedWindowReference();
#else
  HWND oldFocusHwnd;
#endif /* USE_WINUSER */

  PeriodClock enter_clock;
  if (IsEmbedded())
    enter_clock.Update();

  ShowOnTop();

  modal_result = 0;

  SingleWindow &main_window = GetMainWindow();
  main_window.CancelMode();

#ifdef USE_WINUSER
  oldFocusHwnd = ::GetFocus();
#endif /* USE_WINUSER */
  SetDefaultFocus();

  bool hastimed = false;

  main_window.AddDialog(this);

#ifndef USE_GDI
  main_window.Refresh();
#endif

#if defined(ANDROID) || defined(USE_POLL_EVENT) || defined(ENABLE_SDL)
  EventLoop loop(*event_queue, main_window);
#else
  DialogEventLoop loop(*event_queue, *this);
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
    if (IsEmbedded() && !hastimed &&
        event.IsUserInput()) {
      if (!enter_clock.Check(200))
        /* ignore user input in the first 200ms */
        continue;
      else
        hastimed = true;
    }

    if (event.IsKeyDown()) {
      if (OnAnyKeyDown(event.GetKeyCode()))
        continue;

#ifdef ENABLE_SDL
      if (event.GetKeyCode() == SDLK_TAB) {
        /* the Tab key moves the keyboard focus */
        const Uint8 *keystate = ::SDL_GetKeyboardState(nullptr);
        event.event.key.keysym.sym =
            keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]
          ? SDLK_UP : SDLK_DOWN;
      }
#endif

      if (
#ifdef USE_WINUSER
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

#ifndef USE_WINUSER
      if (event.GetKeyCode() == KEY_ESCAPE) {
        modal_result = mrCancel;
        continue;
      }
#endif

#ifdef KOBO
      if (event.GetKeyCode() == KEY_POWER) {
        /* the Kobo power button closes the modal dialog */
        modal_result = mrCancel;
        continue;
      }
#endif
    }

    if (character_function && (event.GetCharacterCount() > 0)) {
      bool handled = false;
      for (size_t i = 0; i < event.GetCharacterCount(); ++i)
        handled = character_function(event.GetCharacter(i)) || handled;
      if (handled)
        continue;
    }

    loop.Dispatch(event);
  } // End Modal Loop

  main_window.RemoveDialog(this);

#ifdef USE_WINUSER
  ::SetFocus(oldFocusHwnd);
#else
  if (old_focus_reference.Defined()) {
    Window *old_focus = old_focus_reference.Get(*root);
    if (old_focus != nullptr)
      old_focus->SetFocus();
  }
#endif /* !USE_WINUSER */

  return modal_result;
}

void
WndForm::OnPaint(Canvas &canvas)
{
  const SingleWindow &main_window = GetMainWindow();
  gcc_unused const bool is_active = main_window.IsTopDialog(*this);

#ifdef ENABLE_OPENGL
  if (!IsDithered() && !IsMaximised() && is_active) {
    /* draw a shade around the current dialog to emphasise it */
    const ScopeAlphaBlend alpha_blend;

    const PixelRect rc = GetClientRect();
    const int size = Layout::VptScale(4);

    const BulkPixelPoint vertices[8] = {
      { rc.left, rc.top },
      { rc.right, rc.top },
      { rc.right, rc.bottom },
      { rc.left, rc.bottom },
      { rc.left - size, rc.top - size },
      { rc.right + size, rc.top - size },
      { rc.right + size, rc.bottom + size },
      { rc.left - size, rc.bottom + size },
    };

    const ScopeVertexPointer vp(vertices);

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

    const ScopeColorPointer cp(colors);

    static constexpr GLubyte indices[] = {
      0, 4, 1, 4, 5, 1,
      1, 5, 2, 5, 6, 2,
      2, 6, 3, 6, 7, 3,
      3, 7, 0, 7, 4, 0,
    };

    glDrawElements(GL_TRIANGLES, ARRAY_SIZE(indices),
                   GL_UNSIGNED_BYTE, indices);
  }
#endif

  ContainerWindow::OnPaint(canvas);

  // Get window coordinates
  PixelRect rcClient = GetClientRect();

  // Draw the borders
  if (!IsMaximised()) {
#ifndef USE_GDI
    if (IsDithered())
      canvas.DrawOutlineRectangle(rcClient.left, rcClient.top,
                                  rcClient.right - 1, rcClient.bottom - 1,
                                  COLOR_BLACK);
    else
#endif
      canvas.DrawRaisedEdge(rcClient);
  }

  if (!caption.empty()) {
    // Set the colors
    canvas.SetTextColor(COLOR_WHITE);

    // Set the titlebar font and font-size
    canvas.Select(*look.caption.font);

    // JMW todo add here icons?

#ifdef EYE_CANDY
    if (!IsDithered() && is_active) {
      canvas.SetBackgroundTransparent();
      canvas.Stretch(title_rect.left, title_rect.top,
                     title_rect.GetWidth(),
                     title_rect.GetHeight(),
                     look.caption.background_bitmap);

      // Draw titlebar text
      canvas.DrawText(title_rect.left + Layout::GetTextPadding(),
                      title_rect.top, caption.c_str());
    } else {
#endif
      canvas.SetBackgroundColor(is_active
                                ? look.caption.background_color
                                : look.caption.inactive_background_color);
      canvas.DrawOpaqueText(title_rect.left + Layout::GetTextPadding(),
                            title_rect.top, title_rect, caption.c_str());
#ifdef EYE_CANDY
    }
#endif
  }

  if (dragging) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    canvas.DrawFilledRectangle(0, 0, canvas.GetWidth(), canvas.GetHeight(),
                               COLOR_YELLOW.WithAlpha(80));
#elif defined(USE_GDI)
    canvas.InvertRectangle(title_rect);
#else
    canvas.InvertRectangle(GetClientRect());
#endif
  }
}

void
WndForm::SetCaption(const TCHAR *_caption)
{
  if (_caption == nullptr)
    _caption = _T("");

  if (!caption.equals(_caption)) {
    caption = _caption;
    UpdateLayout();
    client_area.Move(client_rect);
    Invalidate(title_rect);
  }
}

void
WndForm::ReinitialiseLayout(const PixelRect &parent_rc)
{
  const unsigned parent_width = parent_rc.GetWidth();
  const unsigned parent_height = parent_rc.GetHeight();

  if (parent_width < GetWidth() || parent_height < GetHeight()) {
  } else {
    // reposition dialog to fit into TopWindow
    PixelRect rc = GetPosition();

    if (rc.right > (int)parent_width)
      rc.left = parent_width - rc.GetWidth();
    if (rc.bottom > (int)parent_height)
      rc.top = parent_height - rc.GetHeight();

#ifdef USE_MEMORY_CANVAS
    /* the RasterCanvas class doesn't clip negative window positions
       properly, therefore we avoid this problem at this stage */
    if (rc.left < 0)
      rc.left = 0;
    if (rc.top < 0)
      rc.top = 0;
#endif

    Move(rc.left, rc.top);
  }
}

void
WndForm::SetDefaultFocus()
{
  SetFocus();
  client_area.FocusFirstControl();
}

bool
WndForm::OnAnyKeyDown(unsigned key_code)
{
  return key_down_function && key_down_function(key_code);
}
